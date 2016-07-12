/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "fib.hpp"
#include "pit-entry.hpp"
#include "measurements-entry.hpp"

#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <type_traits>

namespace nfd {
namespace fib {

const unique_ptr<Entry> Fib::s_emptyEntry = make_unique<Entry>(Name());

// http://en.cppreference.com/w/cpp/concept/ForwardIterator
BOOST_CONCEPT_ASSERT((boost::ForwardIterator<Fib::const_iterator>));
// boost::ForwardIterator follows SGI standard http://www.sgi.com/tech/stl/ForwardIterator.html,
// which doesn't require DefaultConstructible
#ifdef HAVE_IS_DEFAULT_CONSTRUCTIBLE
static_assert(std::is_default_constructible<Fib::const_iterator>::value,
              "Fib::const_iterator must be default-constructible");
#else
BOOST_CONCEPT_ASSERT((boost::DefaultConstructible<Fib::const_iterator>));
#endif // HAVE_IS_DEFAULT_CONSTRUCTIBLE

Fib::Fib(NameTree& nameTree)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
}

Fib::~Fib()
{
}

static inline bool
predicate_NameTreeEntry_hasFibEntry(const name_tree::Entry& nte)
{
  return nte.getFibEntry() != nullptr;
}

const Entry&
Fib::findLongestPrefixMatch(const Name& prefix) const
{
  shared_ptr<name_tree::Entry> nte =
    m_nameTree.findLongestPrefixMatch(prefix, &predicate_NameTreeEntry_hasFibEntry);
  if (nte != nullptr) {
    return *nte->getFibEntry();
  }
  return *s_emptyEntry;
}

const Entry&
Fib::findLongestPrefixMatch(shared_ptr<name_tree::Entry> nte) const
{
  Entry* entry = nte->getFibEntry();
  if (entry != nullptr)
    return *entry;

  nte = m_nameTree.findLongestPrefixMatch(nte, &predicate_NameTreeEntry_hasFibEntry);
  if (nte != nullptr) {
    return *nte->getFibEntry();
  }

  return *s_emptyEntry;
}

const Entry&
Fib::findLongestPrefixMatch(const pit::Entry& pitEntry) const
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.findLongestPrefixMatch(pitEntry);
  BOOST_ASSERT(nte != nullptr);
  return findLongestPrefixMatch(nte);
}

const Entry&
Fib::findLongestPrefixMatch(const measurements::Entry& measurementsEntry) const
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.lookup(measurementsEntry);
  BOOST_ASSERT(nte != nullptr);
  return findLongestPrefixMatch(nte);
}

Entry*
Fib::findExactMatch(const Name& prefix)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.findExactMatch(prefix);
  if (nte != nullptr)
    return nte->getFibEntry();

  return nullptr;
}

std::pair<Entry*, bool>
Fib::insert(const Name& prefix)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.lookup(prefix);
  Entry* entry = nte->getFibEntry();
  if (entry != nullptr) {
    return std::make_pair(entry, false);
  }

  nte->setFibEntry(make_unique<Entry>(prefix));
  ++m_nItems;
  return std::make_pair(nte->getFibEntry(), true);
}

void
Fib::erase(shared_ptr<name_tree::Entry> nte)
{
  nte->setFibEntry(nullptr);
  m_nameTree.eraseEntryIfEmpty(nte);
  --m_nItems;
}

void
Fib::erase(const Name& prefix)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.findExactMatch(prefix);
  if (nte != nullptr) {
    this->erase(nte);
  }
}

void
Fib::erase(const Entry& entry)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.lookup(entry);
  if (nte != nullptr) {
    this->erase(nte);
  }
}

void
Fib::removeNextHopFromAllEntries(const Face& face)
{
  std::list<Entry*> toErase;

  auto&& enumerable = m_nameTree.fullEnumerate(&predicate_NameTreeEntry_hasFibEntry);
  for (const name_tree::Entry& nte : enumerable) {
    Entry* entry = nte.getFibEntry();
    entry->removeNextHop(face);
    if (!entry->hasNextHops()) {
      toErase.push_back(entry);
      // entry needs to be erased, but we must wait until the enumeration ends,
      // because otherwise NameTree iterator would be invalidated
    }
  }

  for (Entry* entry : toErase) {
    this->erase(*entry);
  }
}

Fib::const_iterator
Fib::begin() const
{
  return const_iterator(m_nameTree.fullEnumerate(&predicate_NameTreeEntry_hasFibEntry).begin());
}

} // namespace fib
} // namespace nfd
