/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cs.hpp"
#include "core/logger.hpp"
#include "core/algorithm.hpp"
#include <numeric>

NFD_LOG_INIT("ContentStore");

namespace nfd {
namespace cs {

// http://en.cppreference.com/w/cpp/concept/ForwardIterator
BOOST_CONCEPT_ASSERT((boost::ForwardIterator<Cs::const_iterator>));
// boost::ForwardIterator follows SGI standard http://www.sgi.com/tech/stl/ForwardIterator.html,
// which doesn't require DefaultConstructible
#ifdef HAVE_IS_DEFAULT_CONSTRUCTIBLE
static_assert(std::is_default_constructible<Cs::const_iterator>::value,
              "Cs::const_iterator must be default-constructible");
#else
BOOST_CONCEPT_ASSERT((boost::DefaultConstructible<Cs::const_iterator>));
#endif // HAVE_IS_DEFAULT_CONSTRUCTIBLE

Cs::Cs(size_t nMaxPackets)
  : m_limit(nMaxPackets)
{
  BOOST_ASSERT(nMaxPackets > 0);
}

void
Cs::setLimit(size_t nMaxPackets)
{
  BOOST_ASSERT(nMaxPackets > 0);
  m_limit = nMaxPackets;
  this->evict();
}

bool
Cs::insert(const Data& data, bool isUnsolicited)
{
  NFD_LOG_DEBUG("insert " << data.getName());

  // recognize CachingPolicy
  using ndn::nfd::LocalControlHeader;
  const LocalControlHeader& lch = data.getLocalControlHeader();
  if (lch.hasCachingPolicy()) {
    LocalControlHeader::CachingPolicy policy = lch.getCachingPolicy();
    if (policy == LocalControlHeader::CachingPolicy::NO_CACHE) {
      return false;
    }
  }

  bool isNewEntry = false; TableIt it;
  // use .insert because gcc46 does not support .emplace
  std::tie(it, isNewEntry) = m_table.insert(EntryImpl(data.shared_from_this(), isUnsolicited));
  EntryImpl& entry = const_cast<EntryImpl&>(*it);

  if (!isNewEntry) { // existing entry
    this->detachQueue(it);
    // XXX This doesn't forbid unsolicited Data from refreshing a solicited entry.
    if (entry.isUnsolicited() && !isUnsolicited) {
      entry.unsetUnsolicited();
    }
  }
  entry.updateStaleTime();
  this->attachQueue(it);

  // check there are same amount of entries in the table and in queues
  BOOST_ASSERT(m_table.size() == std::accumulate(m_queues, m_queues + QUEUE_MAX, 0U,
      [] (size_t sum, const Queue queue) { return sum + queue.size(); }));

  this->evict(); // XXX The new entry could be evicted, but it shouldn't matter.

  return true;
}

const Data*
Cs::find(const Interest& interest) const
{
  const Name& prefix = interest.getName();
  bool isRightmost = interest.getChildSelector() == 1;
  NFD_LOG_DEBUG("find " << prefix << (isRightmost ? " R" : " L"));

  TableIt first = m_table.lower_bound(prefix);
  TableIt last = m_table.end();
  if (prefix.size() > 0) {
    last = m_table.lower_bound(prefix.getSuccessor());
  }

  TableIt match = last;
  if (isRightmost) {
    match = this->findRightmost(interest, first, last);
  }
  else {
    match = this->findLeftmost(interest, first, last);
  }

  if (match == last) {
    NFD_LOG_DEBUG("  no-match");
    return nullptr;
  }
  NFD_LOG_DEBUG("  matching " << match->getName());
  return &match->getData();
}

TableIt
Cs::findLeftmost(const Interest& interest, TableIt first, TableIt last) const
{
  return std::find_if(first, last, bind(&cs::EntryImpl::canSatisfy, _1, interest));
}

TableIt
Cs::findRightmost(const Interest& interest, TableIt first, TableIt last) const
{
  // Each loop visits a sub-namespace under a prefix one component longer than Interest Name.
  // If there is a match in that sub-namespace, the leftmost match is returned;
  // otherwise, loop continues.

  size_t interestNameLength = interest.getName().size();
  for (TableIt right = last; right != first;) {
    TableIt prev = std::prev(right);

    // special case: [first,prev] have exact Names
    if (prev->getName().size() == interestNameLength) {
      NFD_LOG_TRACE("  find-among-exact " << prev->getName());
      TableIt matchExact = this->findRightmostAmongExact(interest, first, right);
      return matchExact == right ? last : matchExact;
    }

    Name prefix = prev->getName().getPrefix(interestNameLength + 1);
    TableIt left = m_table.lower_bound(prefix);

    // normal case: [left,right) are under one-component-longer prefix
    NFD_LOG_TRACE("  find-under-prefix " << prefix);
    TableIt match = this->findLeftmost(interest, left, right);
    if (match != right) {
      return match;
    }
    right = left;
  }
  return last;
}

TableIt
Cs::findRightmostAmongExact(const Interest& interest, TableIt first, TableIt last) const
{
  return find_last_if(first, last, bind(&EntryImpl::canSatisfy, _1, interest));
}

void
Cs::attachQueue(TableIt it)
{
  EntryImpl& entry = const_cast<EntryImpl&>(*it);

  if (entry.queueType != QUEUE_NONE) {
    this->detachQueue(it);
  }

  if (entry.isUnsolicited()) {
    entry.queueType = QUEUE_UNSOLICITED;
  }
  else if (entry.isStale()) {
    entry.queueType = QUEUE_STALE;
  }
  else {
    entry.queueType = QUEUE_FIFO;

    if (entry.canStale()) {
      entry.moveStaleEvent = scheduler::schedule(entry.getData().getFreshnessPeriod(),
                             bind(&Cs::moveToStaleQueue, this, it));
    }
  }

  Queue& queue = m_queues[entry.queueType];
  entry.queueIt = queue.insert(queue.end(), it);
}

void
Cs::detachQueue(TableIt it)
{
  EntryImpl& entry = const_cast<EntryImpl&>(*it);

  BOOST_ASSERT(entry.queueType != QUEUE_NONE);

  if (entry.queueType == QUEUE_FIFO) {
    scheduler::cancel(entry.moveStaleEvent);
  }

  m_queues[entry.queueType].erase(entry.queueIt);
  entry.queueType = QUEUE_NONE;
}

void
Cs::moveToStaleQueue(TableIt it)
{
  EntryImpl& entry = const_cast<EntryImpl&>(*it);

  BOOST_ASSERT(entry.queueType == QUEUE_FIFO);
  m_queues[QUEUE_FIFO].erase(entry.queueIt);

  entry.queueType = QUEUE_STALE;
  Queue& queue = m_queues[QUEUE_STALE];
  entry.queueIt = queue.insert(queue.end(), it);
}

std::tuple<TableIt, std::string>
Cs::evictPick()
{
  if (!m_queues[QUEUE_UNSOLICITED].empty()) {
    return std::make_tuple(m_queues[QUEUE_UNSOLICITED].front(), "unsolicited");
  }
  if (!m_queues[QUEUE_STALE].empty()) {
    return std::make_tuple(m_queues[QUEUE_STALE].front(), "stale");
  }
  if (!m_queues[QUEUE_FIFO].empty()) {
    return std::make_tuple(m_queues[QUEUE_FIFO].front(), "fifo");
  }

  BOOST_ASSERT(false);
  return std::make_tuple(m_table.end(), "error");
}


void
Cs::evict()
{
  while (this->size() > m_limit) {
    TableIt it; std::string reason;
    std::tie(it, reason) = this->evictPick();

    NFD_LOG_DEBUG("evict " << it->getName() << " " << reason);
    this->detachQueue(it);
    m_table.erase(it);
  }
}

void
Cs::dump()
{
  NFD_LOG_DEBUG("dump table");
  for (const EntryImpl& entry : m_table) {
    NFD_LOG_TRACE(entry.getFullName());
  }
}

} // namespace cs
} // namespace nfd
