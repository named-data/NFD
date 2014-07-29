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

#include "pit-nonce-list.hpp"

namespace nfd {
namespace pit {

// The NonceList has limited capacity to avoid memory explosion
// if PIT entry is constantly refreshed (NFD Bug #1770).
// Current implementation keeps nonces in a set to detect duplicates,
// and a queue to evict the oldest nonce when capacity limit is reached.
// A limitation is that a nonce first appeared at time 0 and duplicated at time 10
// could be evicted before a nonce appeared only once at time 5;
// this limitation should not affect normal operation.

const size_t NonceList::CAPACITY = 256;

NonceList::NonceList()
{
}

bool
NonceList::add(uint32_t nonce)
{
  bool isNew = m_nonceSet.insert(nonce).second;
  if (!isNew)
    return false;

  m_nonceQueue.push(nonce);
  BOOST_ASSERT(m_nonceSet.size() == m_nonceQueue.size());

  if (m_nonceSet.size() > CAPACITY) {
    size_t nErased = m_nonceSet.erase(m_nonceQueue.front());
    BOOST_ASSERT(nErased == 1);
    m_nonceQueue.pop();
    BOOST_ASSERT(m_nonceSet.size() == m_nonceQueue.size());
    BOOST_ASSERT(m_nonceSet.size() <= CAPACITY);
  }
  return true;
}

} // namespace pit
} // namespace nfd
