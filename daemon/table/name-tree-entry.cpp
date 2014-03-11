/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

// Name Tree Entry (i.e., Name Prefix Entry)

#include "name-tree-entry.hpp"

namespace nfd {
namespace name_tree {

Node::Node() : m_prev(0), m_next(0)
{
}

Node::~Node()
{
  // erase the Name Tree Nodes that were created to
  // resolve hash collisions
  // So before erasing a single node, make sure its m_next == 0
  // See eraseEntryIfEmpty in name-tree.cpp
  if (m_next)
    delete m_next;
}

Entry::Entry(const Name& name) : m_hash(0), m_prefix(name)
{
}

Entry::~Entry()
{
}

void
Entry::setHash(uint32_t hash)
{
  m_hash = hash;
}

void
Entry::setParent(shared_ptr<Entry> parent)
{
  m_parent = parent;
}

void
Entry::setFibEntry(shared_ptr<fib::Entry> fibEntry)
{
  if (static_cast<bool>(m_fibEntry)) {
    m_fibEntry->m_nameTreeEntry.reset();
  }
  m_fibEntry = fibEntry;
  if (static_cast<bool>(m_fibEntry)) {
    m_fibEntry->m_nameTreeEntry = this->shared_from_this();
  }
}

void
Entry::insertPitEntry(shared_ptr<pit::Entry> pit)
{
  if (static_cast<bool>(pit)) {
    pit->m_nameTreeEntry = this->shared_from_this();
    m_pitEntries.push_back(pit);
  }
}

bool
Entry::erasePitEntry(shared_ptr<pit::Entry> pit)
{
  for (size_t i = 0; i < m_pitEntries.size(); i++) {
      if (m_pitEntries[i] == pit) {
          BOOST_ASSERT(pit->m_nameTreeEntry);

          pit->m_nameTreeEntry.reset();
          // copy the last item to the current position
          m_pitEntries[i] = m_pitEntries[m_pitEntries.size() - 1];
          // then erase the last item
          m_pitEntries.pop_back();
          return true; // success
      }
  }
  // not found this entry
  return false; // failure
}

void
Entry::setMeasurementsEntry(shared_ptr<measurements::Entry> measurements)
{
  if (static_cast<bool>(m_measurementsEntry)) {
    m_measurementsEntry->m_nameTreeEntry.reset();
  }
  m_measurementsEntry = measurements;
  if (static_cast<bool>(m_measurementsEntry)) {
    m_measurementsEntry->m_nameTreeEntry = this->shared_from_this();
  }
}

void
Entry::setStrategyChoiceEntry(shared_ptr<strategy_choice::Entry> strategyChoiceEntry)
{
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
