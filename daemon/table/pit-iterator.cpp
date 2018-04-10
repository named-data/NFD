/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "pit-iterator.hpp"

#include <ndn-cxx/util/concepts.hpp>

namespace nfd {
namespace pit {

NDN_CXX_ASSERT_FORWARD_ITERATOR(Iterator);

Iterator::Iterator(const NameTree::const_iterator& ntIt, size_t iPitEntry)
  : m_ntIt(ntIt)
  , m_iPitEntry(iPitEntry)
{
}

Iterator&
Iterator::operator++()
{
  BOOST_ASSERT(m_ntIt != NameTree::const_iterator());
  BOOST_ASSERT(m_iPitEntry < m_ntIt->getPitEntries().size());

  if (++m_iPitEntry >= m_ntIt->getPitEntries().size()) {
    ++m_ntIt;
    m_iPitEntry = 0;
    BOOST_ASSERT(m_ntIt == NameTree::const_iterator() || m_ntIt->hasPitEntries());
  }

  return *this;
}

Iterator
Iterator::operator++(int)
{
  Iterator copy = *this;
  this->operator++();
  return copy;
}

} // namespace pit
} // namespace nfd
