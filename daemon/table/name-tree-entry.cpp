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

#include "name-tree-entry.hpp"

namespace nfd {
namespace name_tree {

Node::Node()
  : m_prev(0)
  , m_next(0)
{
}

Node::~Node()
{
  // erase the Name Tree Nodes that were created to
  // resolve hash collisions
  // So before erasing a single node, make sure its m_next == 0
  // See eraseEntryIfEmpty in name-tree.cpp
  if (m_next != 0)
    delete m_next;
}

Entry::Entry(const Name& name)
  : m_hash(0)
  , m_prefix(name)
{
}

Entry::~Entry()
{
}

bool
Entry::isEmpty() const
{
  return m_children.empty() &&
         !static_cast<bool>(m_fibEntry) &&
         m_pitEntries.empty() &&
         !static_cast<bool>(m_measurementsEntry) &&
         !static_cast<bool>(m_strategyChoiceEntry);
}

void
Entry::setFibEntry(shared_ptr<fib::Entry> fibEntry)
{
  if (static_cast<bool>(fibEntry)) {
    BOOST_ASSERT(!static_cast<bool>(fibEntry->m_nameTreeEntry));
  }

  if (static_cast<bool>(m_fibEntry)) {
    m_fibEntry->m_nameTreeEntry.reset();
  }
  m_fibEntry = fibEntry;
  if (static_cast<bool>(m_fibEntry)) {
    m_fibEntry->m_nameTreeEntry = this->shared_from_this();
  }
}

void
Entry::insertPitEntry(shared_ptr<pit::Entry> pitEntry)
{
  BOOST_ASSERT(static_cast<bool>(pitEntry));
  BOOST_ASSERT(!static_cast<bool>(pitEntry->m_nameTreeEntry));

  m_pitEntries.push_back(pitEntry);
  pitEntry->m_nameTreeEntry = this->shared_from_this();
}

void
Entry::erasePitEntry(shared_ptr<pit::Entry> pitEntry)
{
  BOOST_ASSERT(static_cast<bool>(pitEntry));
  BOOST_ASSERT(pitEntry->m_nameTreeEntry.get() == this);

  std::vector<shared_ptr<pit::Entry> >::iterator it =
    std::find(m_pitEntries.begin(), m_pitEntries.end(), pitEntry);
  BOOST_ASSERT(it != m_pitEntries.end());

  *it = m_pitEntries.back();
  m_pitEntries.pop_back();
  pitEntry->m_nameTreeEntry.reset();
}

void
Entry::setMeasurementsEntry(shared_ptr<measurements::Entry> measurementsEntry)
{
  if (static_cast<bool>(measurementsEntry)) {
    BOOST_ASSERT(!static_cast<bool>(measurementsEntry->m_nameTreeEntry));
  }

  if (static_cast<bool>(m_measurementsEntry)) {
    m_measurementsEntry->m_nameTreeEntry.reset();
  }
  m_measurementsEntry = measurementsEntry;
  if (static_cast<bool>(m_measurementsEntry)) {
    m_measurementsEntry->m_nameTreeEntry = this->shared_from_this();
  }
}

void
Entry::setStrategyChoiceEntry(shared_ptr<strategy_choice::Entry> strategyChoiceEntry)
{
  if (static_cast<bool>(strategyChoiceEntry)) {
    BOOST_ASSERT(!static_cast<bool>(strategyChoiceEntry->m_nameTreeEntry));
  }

  if (static_cast<bool>(m_strategyChoiceEntry)) {
    m_strategyChoiceEntry->m_nameTreeEntry.reset();
  }
  m_strategyChoiceEntry = strategyChoiceEntry;
  if (static_cast<bool>(m_strategyChoiceEntry)) {
    m_strategyChoiceEntry->m_nameTreeEntry = this->shared_from_this();
  }
}

} // namespace name_tree
} // namespace nfd
