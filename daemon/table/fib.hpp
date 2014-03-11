/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_FIB_HPP
#define NFD_TABLE_FIB_HPP

#include "fib-entry.hpp"
#include "name-tree.hpp"

namespace nfd {

namespace measurements {
class Entry;
}
namespace pit {
class Entry;
}

/** \class Fib
 *  \brief represents the FIB
 */
class Fib : noncopyable
{
public:
  explicit
  Fib(NameTree& nameTree);

  ~Fib();

  /** \brief inserts a FIB entry for prefix
   *  If an entry for exact same prefix exists, that entry is returned.
   *  \return{ the entry, and true for new entry, false for existing entry }
   */
  std::pair<shared_ptr<fib::Entry>, bool>
  insert(const Name& prefix);

  /// performs a longest prefix match
  shared_ptr<fib::Entry>
  findLongestPrefixMatch(const Name& prefix) const;

  /// performs a longest prefix match
  shared_ptr<fib::Entry>
  findLongestPrefixMatch(const pit::Entry& pitEntry) const;

  /// performs a longest prefix match
  shared_ptr<fib::Entry>
  findLongestPrefixMatch(const measurements::Entry& measurementsEntry) const;

  shared_ptr<fib::Entry>
  findExactMatch(const Name& prefix) const;

  void
  erase(const Name& prefix);

  /** \brief removes the NextHop record for face in all entrites
   *  This is usually invoked when face goes away.
   *  Removing all NextHops in a FIB entry will not remove the FIB entry.
   */
  void
  removeNextHopFromAllEntries(shared_ptr<Face> face);

  size_t
  size() const;

private:
  void
  erase(const fib::Entry& entry);

  shared_ptr<fib::Entry>
  findLongestPrefixMatch(shared_ptr<name_tree::Entry> nameTreeEntry) const;

private:
  NameTree& m_nameTree;
  size_t m_nItems;

  /** \brief The empty FIB entry.
   *
   *  This entry has no nexthops.
   *  It is returned by findLongestPrefixMatch if nothing is matched.
   */
  // Returning empty entry instead of nullptr makes forwarding and strategy implementation easier.
  static const shared_ptr<fib::Entry> s_emptyEntry;
};

inline size_t
Fib::size() const
{
  return m_nItems;
}

} // namespace nfd

#endif // NFD_TABLE_FIB_HPP
