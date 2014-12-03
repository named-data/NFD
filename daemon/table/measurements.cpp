/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

Measurements::Measurements(NameTree& nameTree)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
}

shared_ptr<measurements::Entry>
Measurements::get(name_tree::Entry& nte)
{
  shared_ptr<measurements::Entry> entry = nte.getMeasurementsEntry();
  if (entry != nullptr)
    return entry;

  entry = make_shared<measurements::Entry>(nte.getPrefix());
  nte.setMeasurementsEntry(entry);
  ++m_nItems;

  entry->m_expiry = time::steady_clock::now() + getInitialLifetime();
  entry->m_cleanup = scheduler::schedule(getInitialLifetime(),
                                         bind(&Measurements::cleanup, this, ref(*entry)));

  return entry;
}

shared_ptr<measurements::Entry>
Measurements::get(const Name& name)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.lookup(name);
  return this->get(*nte);
}

shared_ptr<measurements::Entry>
Measurements::get(const fib::Entry& fibEntry)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.get(fibEntry);
  return this->get(*nte);
}

shared_ptr<measurements::Entry>
Measurements::get(const pit::Entry& pitEntry)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.get(pitEntry);
  return this->get(*nte);
}

shared_ptr<measurements::Entry>
Measurements::getParent(const measurements::Entry& child)
{
  if (child.getName().size() == 0) { // the root entry
    return nullptr;
  }

  shared_ptr<name_tree::Entry> nteChild = m_nameTree.get(child);
  shared_ptr<name_tree::Entry> nte = nteChild->getParent();
  BOOST_ASSERT(nte != nullptr);
  return this->get(*nte);
}

shared_ptr<measurements::Entry>
Measurements::findLongestPrefixMatch(const Name& name) const
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.findLongestPrefixMatch(name,
      [] (const name_tree::Entry& nte) { return nte.getMeasurementsEntry() != nullptr; });
  if (nte != nullptr) {
    return nte->getMeasurementsEntry();
  }
  return nullptr;
}

shared_ptr<measurements::Entry>
Measurements::findExactMatch(const Name& name) const
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.lookup(name);
  if (nte != nullptr)
    return nte->getMeasurementsEntry();
  return nullptr;
}

void
Measurements::extendLifetime(measurements::Entry& entry,
                             const time::nanoseconds& lifetime)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.get(entry);
  if (nte == nullptr ||
      nte->getMeasurementsEntry().get() != &entry) {
    // entry is already gone; it is a dangling reference
    return;
  }

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
Measurements::cleanup(measurements::Entry& entry)
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.get(entry);
  if (nte != nullptr) {
    nte->setMeasurementsEntry(nullptr);
    m_nameTree.eraseEntryIfEmpty(nte);
    m_nItems--;
  }
}

} // namespace nfd
