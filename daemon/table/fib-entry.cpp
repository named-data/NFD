/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fib-entry.hpp"

namespace nfd {
namespace fib {

Entry::Entry(const Name& prefix)
  : m_prefix(prefix)
{
}

static inline bool
predicate_NextHop_eq_Face(const NextHop& nexthop, shared_ptr<Face> face)
{
  return nexthop.getFace() == face;
}

bool
Entry::hasNextHop(shared_ptr<Face> face) const
{
  NextHopList::const_iterator it = std::find_if(m_nextHops.begin(), m_nextHops.end(),
    bind(&predicate_NextHop_eq_Face, _1, face));
  return it != m_nextHops.end();
}

void
Entry::addNextHop(shared_ptr<Face> face, int32_t cost)
{
  NextHopList::iterator it = std::find_if(m_nextHops.begin(), m_nextHops.end(),
    bind(&predicate_NextHop_eq_Face, _1, face));
  if (it == m_nextHops.end()) {
    m_nextHops.push_back(fib::NextHop(face));
    it = m_nextHops.end() - 1;
  }
  // now it refers to the NextHop for face

  it->setCost(cost);

  this->sortNextHops();
}

void
Entry::removeNextHop(shared_ptr<Face> face)
{
  NextHopList::iterator it = std::find_if(m_nextHops.begin(), m_nextHops.end(),
    bind(&predicate_NextHop_eq_Face, _1, face));
  if (it == m_nextHops.end()) {
    return;
  }

  m_nextHops.erase(it);
}

static inline bool
compare_NextHop_cost(const NextHop& a, const NextHop& b)
{
  return a.getCost() < b.getCost();
}

void
Entry::sortNextHops()
{
  std::sort(m_nextHops.begin(), m_nextHops.end(), &compare_NextHop_cost);
}


} // namespace fib
} // namespace nfd
