/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "measurements.hpp"
#include "name-tree.hpp"
#include "pit-entry.hpp"
#include "fib-entry.hpp"

namespace nfd {
namespace measurements {

Measurements::Measurements(NameTree& nameTree)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
}

Entry&
Measurements::get(name_tree::Entry& nte)
{
  Entry* entry = nte.getMeasurementsEntry();
  if (entry != nullptr) {
    return *entry;
  }

  nte.setMeasurementsEntry(make_unique<Entry>(nte.getName()));
  ++m_nItems;
  entry = nte.getMeasurementsEntry();

  entry->m_expiry = time::steady_clock::now() + getInitialLifetime();
  entry->m_cleanup = scheduler::schedule(getInitialLifetime(),
                                         bind(&Measurements::cleanup, this, ref(*entry)));

  return *entry;
}

Entry&
Measurements::get(const Name& name)
{
  name_tree::Entry& nte = m_nameTree.lookup(name, std::min(name.size(), getMaxDepth()));
  return this->get(nte);
}

Entry&
Measurements::get(const fib::Entry& fibEntry)
{
  name_tree::Entry& nte = m_nameTree.lookup(fibEntry);
  return this->get(nte);
}

Entry&
Measurements::get(const pit::Entry& pitEntry)
{
  name_tree::Entry& nte = m_nameTree.lookup(pitEntry);
  return this->get(nte);
}

Entry*
Measurements::getParent(const Entry& child)
{
  if (child.getName().empty()) { // the root entry
    return nullptr;
  }

  name_tree::Entry* nteChild = m_nameTree.getEntry(child);
  name_tree::Entry* nte = nteChild->getParent();
  BOOST_ASSERT(nte != nullptr);
  return &this->get(*nte);
}

template<typename K>
Entry*
Measurements::findLongestPrefixMatchImpl(const K& key, const EntryPredicate& pred) const
{
  name_tree::Entry* match = m_nameTree.findLongestPrefixMatch(key,
    [&pred] (const name_tree::Entry& nte) {
      const Entry* entry = nte.getMeasurementsEntry();
      return entry != nullptr && pred(*entry);
    });
  if (match != nullptr) {
    return match->getMeasurementsEntry();
  }
  return nullptr;
}

Entry*
Measurements::findLongestPrefixMatch(const Name& name, const EntryPredicate& pred) const
{
  return this->findLongestPrefixMatchImpl(name.getPrefix(NameTree::getMaxDepth()), pred);
}

Entry*
Measurements::findLongestPrefixMatch(const pit::Entry& pitEntry, const EntryPredicate& pred) const
{
  return this->findLongestPrefixMatch(pitEntry.getName(), pred);
}

Entry*
Measurements::findExactMatch(const Name& name) const
{
  const name_tree::Entry* nte = m_nameTree.findExactMatch(name);
  return nte == nullptr ? nullptr : nte->getMeasurementsEntry();
}

void
Measurements::extendLifetime(Entry& entry, const time::nanoseconds& lifetime)
{
  BOOST_ASSERT(m_nameTree.getEntry(entry) != nullptr);

  time::steady_clock::TimePoint expiry = time::steady_clock::now() + lifetime;
  if (entry.m_expiry >= expiry) {
    // has longer lifetime, not extending
    return;
  }

  scheduler::cancel(entry.m_cleanup);
  entry.m_expiry = expiry;
  entry.m_cleanup = scheduler::schedule(lifetime, bind(&Measurements::cleanup, this, ref(entry)));
}

void
Measurements::cleanup(Entry& entry)
{
  name_tree::Entry* nte = m_nameTree.getEntry(entry);
  BOOST_ASSERT(nte != nullptr);

  nte->setMeasurementsEntry(nullptr);
  m_nameTree.eraseIfEmpty(nte);
  --m_nItems;
}

} // namespace measurements
} // namespace nfd
