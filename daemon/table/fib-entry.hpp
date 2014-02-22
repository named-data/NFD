/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_FIB_ENTRY_HPP
#define NFD_TABLE_FIB_ENTRY_HPP

#include "fib-nexthop.hpp"

namespace nfd {

namespace fw {
class Strategy;
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
class Entry : public StrategyInfoHost, noncopyable
{
public:
  explicit
  Entry(const Name& prefix);
  
  const Name&
  getPrefix() const;
  
  /** \brief gives the nexthops explicitly defined on this entry
   *  This list does not include inherited nexthops.
   */
  const NextHopList&
  getNextHops() const;
  
  /// adds a nexthop
  void
  addNextHop(shared_ptr<Face> face, int32_t cost);
  
  /// removes a nexthop
  void
  removeNextHop(shared_ptr<Face> face);
  
  const fw::Strategy&
  getStrategy() const;
  
  void
  setStrategy(shared_ptr<fw::Strategy> strategy);

private:
  /// sorts the nexthop list
  void
  sortNextHops();

private:
  Name m_prefix;
  NextHopList m_nextHops;
  shared_ptr<fw::Strategy> m_strategy;
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

inline const fw::Strategy&
Entry::getStrategy() const
{
  BOOST_ASSERT(static_cast<bool>(m_strategy));
  return *m_strategy;
}

} // namespace fib
} // namespace nfd

#endif // NFD_TABLE_FIB_ENTRY_HPP
