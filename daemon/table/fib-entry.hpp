/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_FIB_ENTRY_HPP
#define NFD_TABLE_FIB_ENTRY_HPP

#include "fib-nexthop.hpp"

namespace nfd {

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
class Entry : public StrategyInfoHost, noncopyable
{
public:
  explicit
  Entry(const Name& prefix);

  const Name&
  getPrefix() const;

  const NextHopList&
  getNextHops() const;

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

} // namespace fib
} // namespace nfd

#endif // NFD_TABLE_FIB_ENTRY_HPP
