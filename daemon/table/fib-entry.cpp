/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "fib-entry.hpp"

namespace nfd::fib {

Entry::Entry(const Name& prefix)
  : m_prefix(prefix)
{
}

NextHopList::iterator
Entry::findNextHop(const Face& face) noexcept
{
  return std::find_if(m_nextHops.begin(), m_nextHops.end(),
                      [&face] (const NextHop& nexthop) { return &nexthop.getFace() == &face; });
}

bool
Entry::hasNextHop(const Face& face) const noexcept
{
  return const_cast<Entry*>(this)->findNextHop(face) != m_nextHops.end();
}

std::pair<NextHopList::iterator, bool>
Entry::addOrUpdateNextHop(Face& face, uint64_t cost)
{
  auto it = findNextHop(face);
  bool isNew = false;
  if (it == m_nextHops.end()) {
    m_nextHops.emplace_back(face);
    it = std::prev(m_nextHops.end());
    isNew = true;
  }

  it->setCost(cost);
  this->sortNextHops();

  return {it, isNew};
}

bool
Entry::removeNextHop(const Face& face)
{
  auto it = findNextHop(face);
  if (it != m_nextHops.end()) {
    m_nextHops.erase(it);
    return true;
  }
  return false;
}

void
Entry::sortNextHops()
{
  std::sort(m_nextHops.begin(), m_nextHops.end(),
            [] (const NextHop& a, const NextHop& b) { return a.getCost() < b.getCost(); });
}

} // namespace nfd::fib
