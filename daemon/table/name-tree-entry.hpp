/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

// Name Tree Entry (i.e., Name Prefix Entry)

#ifndef NFD_TABLE_NAME_TREE_ENTRY_HPP
#define NFD_TABLE_NAME_TREE_ENTRY_HPP

#include "common.hpp"
#include "table/fib-entry.hpp"
#include "table/pit-entry.hpp"
#include "table/measurements-entry.hpp"

namespace nfd {

class NameTree;

namespace name_tree {

// Forward declaration
class Node;
class Entry;

/**
 * @brief Name Tree Node Class
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
 * @brief Name Tree Entry Class
 */
class Entry
{
  // Make private members accessible by Name Tree
  friend class nfd::NameTree;
public:
  explicit
  Entry(const Name& prefix);

  ~Entry();

  const Name&
  getPrefix() const;

  void
  setHash(uint32_t hash);

  uint32_t
  getHash() const;

  void
  setParent(shared_ptr<Entry> parent);

  shared_ptr<Entry>
  getParent() const;

  std::vector<shared_ptr<Entry> >&
  getChildren();
  
  bool
  isEmpty() const;

  void
  setFibEntry(shared_ptr<fib::Entry> fib);

  shared_ptr<fib::Entry>
  getFibEntry() const;

  bool
  eraseFibEntry(shared_ptr<fib::Entry> fib);

  void
  insertPitEntry(shared_ptr<pit::Entry> pit);

  std::vector<shared_ptr<pit::Entry> >&
  getPitEntries();

  /**
   * @brief Erase a PIT Entry
   * @details The address of this PIT Entry is required
   */
  bool
  erasePitEntry(shared_ptr<pit::Entry> pit);

  void
  setMeasurementsEntry(shared_ptr<measurements::Entry> measurements);

  shared_ptr<measurements::Entry>
  getMeasurementsEntry() const;

  bool
  eraseMeasurementsEntry(shared_ptr<measurements::Entry> measurements);

private:
  uint32_t m_hash;
  Name m_prefix;
  shared_ptr<Entry> m_parent;     // Pointing to the parent entry.
  std::vector<shared_ptr<Entry> > m_children; // Children pointers.
  shared_ptr<fib::Entry> m_fibEntry;
  std::vector<shared_ptr<pit::Entry> > m_pitEntries;
  shared_ptr<measurements::Entry> m_measurementsEntry;

  // get the Name Tree Node that is associated with this Name Tree Entry
  Node* m_node;
};

inline const Name&
Entry::getPrefix() const
{
  return m_prefix;
}

inline uint32_t
Entry::getHash() const
{
  return m_hash;
}

inline shared_ptr<Entry>
Entry::getParent() const
{
  return m_parent;
}

inline std::vector<shared_ptr<name_tree::Entry> >&
Entry::getChildren()
{
  return m_children;
}

inline bool
Entry::isEmpty() const
{
  return m_children.empty() &&
         !static_cast<bool>(m_fibEntry) &&
         m_pitEntries.empty() &&
         !static_cast<bool>(m_measurementsEntry);
}

inline shared_ptr<fib::Entry>
Entry::getFibEntry() const
{
  return m_fibEntry;
}

inline std::vector<shared_ptr<pit::Entry> >&
Entry::getPitEntries()
{
  return m_pitEntries;
}

inline shared_ptr<measurements::Entry>
Entry::getMeasurementsEntry() const
{
  return m_measurementsEntry;
}

} // namespace name_tree
} // namespace nfd

#endif // NFD_TABLE_NAME_TREE_ENTRY_HPP
