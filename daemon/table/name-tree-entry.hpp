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

#ifndef NFD_DAEMON_TABLE_NAME_TREE_ENTRY_HPP
#define NFD_DAEMON_TABLE_NAME_TREE_ENTRY_HPP

#include "table/fib-entry.hpp"
#include "table/pit-entry.hpp"
#include "table/measurements-entry.hpp"
#include "table/strategy-choice-entry.hpp"

namespace nfd {
namespace name_tree {

class Node;

/** \brief an entry in the name tree
 */
class Entry : noncopyable
{
public:
  Entry(const Name& prefix, Node* node);

  const Name&
  getName() const
  {
    return m_name;
  }

  /** \return entry of getName().getPrefix(-1)
   *  \retval nullptr this entry is the root entry, i.e. getName() == Name()
   */
  Entry*
  getParent() const
  {
    return m_parent;
  }

  /** \brief set parent of this entry
   *  \param entry entry of getName().getPrefix(-1)
   *  \pre getParent() == nullptr
   *  \post getParent() == &entry
   *  \post entry.getChildren() contains this
   */
  void
  setParent(Entry& entry);

  /** \brief unset parent of this entry
   *  \post getParent() == nullptr
   *  \post parent.getChildren() does not contain this
   */
  void
  unsetParent();

  /** \retval true this entry has at least one child
   *  \retval false this entry has no children
   */
  bool
  hasChildren() const
  {
    return !this->getChildren().empty();
  }

  /** \return children of this entry
   */
  const std::vector<Entry*>&
  getChildren() const
  {
    return m_children;
  }

  /** \retval true this entry has no children and no table entries
   *  \retval false this entry has child or attached table entry
   */
  bool
  isEmpty() const
  {
    return !this->hasChildren() && !this->hasTableEntries();
  }

public: // attached table entries
  /** \retval true at least one table entries is attached
   *  \retval false no table entry is attached
   */
  bool
  hasTableEntries() const;

  fib::Entry*
  getFibEntry() const
  {
    return m_fibEntry.get();
  }

  void
  setFibEntry(unique_ptr<fib::Entry> fibEntry);

  bool
  hasPitEntries() const
  {
    return !this->getPitEntries().empty();
  }

  const std::vector<shared_ptr<pit::Entry>>&
  getPitEntries() const
  {
    return m_pitEntries;
  }

  void
  insertPitEntry(shared_ptr<pit::Entry> pitEntry);

  void
  erasePitEntry(pit::Entry* pitEntry);

  measurements::Entry*
  getMeasurementsEntry() const
  {
    return m_measurementsEntry.get();
  }

  void
  setMeasurementsEntry(unique_ptr<measurements::Entry> measurementsEntry);

  strategy_choice::Entry*
  getStrategyChoiceEntry() const
  {
    return m_strategyChoiceEntry.get();
  }

  void
  setStrategyChoiceEntry(unique_ptr<strategy_choice::Entry> strategyChoiceEntry);

  /** \return name tree entry on which a table entry is attached,
   *          or nullptr if the table entry is detached
   *  \note This function is for NameTree internal use. Other components
   *        should use NameTree::getEntry(tableEntry) instead.
   */
  template<typename ENTRY>
  static Entry*
  get(const ENTRY& tableEntry)
  {
    return tableEntry.m_nameTreeEntry;
  }

private:
  Name m_name;
  Node* m_node;
  Entry* m_parent;
  std::vector<Entry*> m_children;

  unique_ptr<fib::Entry> m_fibEntry;
  std::vector<shared_ptr<pit::Entry>> m_pitEntries;
  unique_ptr<measurements::Entry> m_measurementsEntry;
  unique_ptr<strategy_choice::Entry> m_strategyChoiceEntry;

  friend Node* getNode(const Entry& entry);
};

/** \brief a functor to get a table entry from a name tree entry
 *  \tparam ENTRY type of single table entry attached to name tree entry, such as fib::Entry
 */
template<typename ENTRY>
class GetTableEntry
{
public:
  /** \brief a function pointer to the getter on Entry class that returns ENTRY
   */
  using Getter = ENTRY* (Entry::*)() const;

  /** \note The default argument is needed to ensure FIB and StrategyChoice iterators
   *        are default-constructible.
   */
  explicit
  GetTableEntry(Getter getter = nullptr)
    : m_getter(getter)
  {
  }

  const ENTRY&
  operator()(const Entry& nte) const
  {
    return *(nte.*m_getter)();
  }

private:
  Getter m_getter;
};

} // namespace name_tree
} // namespace nfd

#endif // NFD_DAEMON_TABLE_NAME_TREE_ENTRY_HPP
