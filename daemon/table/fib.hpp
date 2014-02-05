/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_FIB_HPP
#define NFD_TABLE_FIB_HPP

#include "fib-entry.hpp"
namespace nfd {

/** \class Fib
 *  \brief represents the FIB
 */
class Fib : noncopyable
{
public:
  Fib();

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

  shared_ptr<fib::Entry>
  findExactMatch(const Name& prefix) const;

  void
  remove(const Name& prefix);

  /** \brief removes the NextHop record for face in all entrites
   *  This is usually invoked when face goes away.
   *  Removing all NextHops in a FIB entry will not remove the FIB entry.
   */
  void
  removeNextHopFromAllEntries(shared_ptr<Face> face);

private:
  shared_ptr<fib::Entry> m_rootEntry;
  std::list<shared_ptr<fib::Entry> > m_table;
};

} // namespace nfd

#endif // NFD_TABLE_FIB_HPP
