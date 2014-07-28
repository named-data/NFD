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

#ifndef NFD_DAEMON_TABLE_NAME_TREE_ENTRY_HPP
#define NFD_DAEMON_TABLE_NAME_TREE_ENTRY_HPP

#include "common.hpp"
#include "table/fib-entry.hpp"
#include "table/pit-entry.hpp"
#include "table/measurements-entry.hpp"
#include "table/strategy-choice-entry.hpp"

namespace nfd {

class NameTree;

namespace name_tree {

// Forward declarations
class Node;
class Entry;

/**
 * \brief Name Tree Node Class
 */
class Node
{
public:
  Node();

  ~Node();

public:
  // variables are in public as this is just a data structure
  shared_ptr<Entry> m_entry; // Name Tree Entry (i.e., Name Prefix Entry)
  Node* m_prev; // Previous Name Tree Node (to resolve hash collision)
  Node* m_next; // Next Name Tree Node (to resolve hash collision)
};

/**
 * \brief Name Tree Entry Class
 */
class Entry : public enable_shared_from_this<Entry>, noncopyable
{
public:
  explicit
  Entry(const Name& prefix);

  ~Entry();

  const Name&
  getPrefix() const;

  void
  setHash(size_t hash);

  size_t
  getHash() const;

  void
  setParent(shared_ptr<Entry> parent);

  shared_ptr<Entry>
  getParent() const;

  std::vector<shared_ptr<Entry> >&
  getChildren();

  bool
  hasChildren() const;

  bool
  isEmpty() const;

public: // attached table entries
  void
  setFibEntry(shared_ptr<fib::Entry> fibEntry);

  shared_ptr<fib::Entry>
  getFibEntry() const;

  void
  insertPitEntry(shared_ptr<pit::Entry> pitEntry);

  void
  erasePitEntry(shared_ptr<pit::Entry> pitEntry);

  bool
  hasPitEntries() const;

  const std::vector<shared_ptr<pit::Entry> >&
  getPitEntries() const;

  void
  setMeasurementsEntry(shared_ptr<measurements::Entry> measurementsEntry);

  shared_ptr<measurements::Entry>
  getMeasurementsEntry() const;

  void
  setStrategyChoiceEntry(shared_ptr<strategy_choice::Entry> strategyChoiceEntry);

  shared_ptr<strategy_choice::Entry>
  getStrategyChoiceEntry() const;

private:
  // Benefits of storing m_hash
  // 1. m_hash is compared before m_prefix is compared
  // 2. fast hash table resize support
  size_t m_hash;
  Name m_prefix;
  shared_ptr<Entry> m_parent;     // Pointing to the parent entry.
  std::vector<shared_ptr<Entry> > m_children; // Children pointers.
  shared_ptr<fib::Entry> m_fibEntry;
  std::vector<shared_ptr<pit::Entry> > m_pitEntries;
  shared_ptr<measurements::Entry> m_measurementsEntry;
  shared_ptr<strategy_choice::Entry> m_strategyChoiceEntry;

  // get the Name Tree Node that is associated with this Name Tree Entry
  Node* m_node;

  // Make private members accessible by Name Tree
  friend class nfd::NameTree;
};

inline const Name&
Entry::getPrefix() const
{
  return m_prefix;
}

inline size_t
Entry::getHash() const
{
  return m_hash;
}

inline void
Entry::setHash(size_t hash)
{
  m_hash = hash;
}

inline shared_ptr<Entry>
Entry::getParent() const
{
  return m_parent;
}

inline void
Entry::setParent(shared_ptr<Entry> parent)
{
  m_parent = parent;
}

inline std::vector<shared_ptr<name_tree::Entry> >&
Entry::getChildren()
{
  return m_children;
}

inline bool
Entry::hasChildren() const
{
  return !m_children.empty();
}

inline shared_ptr<fib::Entry>
Entry::getFibEntry() const
{
  return m_fibEntry;
}

inline bool
Entry::hasPitEntries() const
{
  return !m_pitEntries.empty();
}

inline const std::vector<shared_ptr<pit::Entry> >&
Entry::getPitEntries() const
{
  return m_pitEntries;
}

inline shared_ptr<measurements::Entry>
Entry::getMeasurementsEntry() const
{
  return m_measurementsEntry;
}

inline shared_ptr<strategy_choice::Entry>
Entry::getStrategyChoiceEntry() const
{
  return m_strategyChoiceEntry;
}

} // namespace name_tree
} // namespace nfd

#endif // NFD_DAEMON_TABLE_NAME_TREE_ENTRY_HPP
