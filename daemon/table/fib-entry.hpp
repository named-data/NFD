/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_FIB_ENTRY_HPP
#define NFD_TABLE_FIB_ENTRY_HPP

#include "fib-nexthop.hpp"

namespace nfd {

class NameTree;
namespace name_tree {
class Entry;
}

namespace fib {

/** \class NextHopList
 *  \brief represents a collection of nexthops
 *  This type has these methods:
 *    iterator<NextHop> begin()
 *    iterator<NextHop> end()
 *    size_t size()
 */
typedef std::vector<fib::NextHop> NextHopList;

/** \class Entry
 *  \brief represents a FIB entry
 */
class Entry : noncopyable
{
public:
  explicit
  Entry(const Name& prefix);

  const Name&
  getPrefix() const;

  const NextHopList&
  getNextHops() const;

  /// whether this Entry has any nexthop
  bool
  hasNextHops() const;

  bool
  hasNextHop(shared_ptr<Face> face) const;

  /// adds a nexthop
  void
  addNextHop(shared_ptr<Face> face, int32_t cost);

  /// removes a nexthop
  void
  removeNextHop(shared_ptr<Face> face);

private:
  /// sorts the nexthop list
  void
  sortNextHops();

private:
  Name m_prefix;
  NextHopList m_nextHops;

  shared_ptr<name_tree::Entry> m_nameTreeEntry;
  friend class nfd::NameTree;
  friend class nfd::name_tree::Entry;
};


inline const Name&
Entry::getPrefix() const
{
  return m_prefix;
}

inline const NextHopList&
Entry::getNextHops() const
{
  return m_nextHops;
}

inline bool
Entry::hasNextHops() const
{
  return !m_nextHops.empty();
}

} // namespace fib
} // namespace nfd

#endif // NFD_TABLE_FIB_ENTRY_HPP
