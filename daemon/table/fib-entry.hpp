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

#ifndef NFD_DAEMON_TABLE_FIB_ENTRY_HPP
#define NFD_DAEMON_TABLE_FIB_ENTRY_HPP

#include "core/common.hpp"

namespace nfd {

namespace face {
class Face;
} // namespace face
using face::Face;

namespace name_tree {
class Entry;
} // namespace name_tree

namespace fib {

class Fib;

/**
 * \brief Represents a nexthop record in a FIB entry.
 */
class NextHop
{
public:
  explicit
  NextHop(Face& face) noexcept
    : m_face(&face)
  {
  }

  Face&
  getFace() const noexcept
  {
    return *m_face;
  }

  uint64_t
  getCost() const noexcept
  {
    return m_cost;
  }

  void
  setCost(uint64_t cost) noexcept
  {
    m_cost = cost;
  }

private:
  Face* m_face; // pointer instead of reference so that NextHop is movable
  uint64_t m_cost = 0;
};

/**
 * \brief A collection of nexthops in a FIB entry.
 */
using NextHopList = std::vector<NextHop>;

/**
 * \brief Represents an entry in the FIB.
 * \sa Fib
 */
class Entry : noncopyable
{
public:
  explicit
  Entry(const Name& prefix);

  const Name&
  getPrefix() const noexcept
  {
    return m_prefix;
  }

  const NextHopList&
  getNextHops() const noexcept
  {
    return m_nextHops;
  }

  /**
   * \brief Returns whether this Entry has any NextHop records.
   */
  bool
  hasNextHops() const noexcept
  {
    return !m_nextHops.empty();
  }

  /**
   * \brief Returns whether there is a NextHop record for \p face.
   */
  bool
  hasNextHop(const Face& face) const noexcept;

private:
  /** \brief Adds a NextHop record to the entry.
   *
   *  If a NextHop record for \p face already exists in the entry, its cost is set to \p cost.
   *
   *  \return the iterator to the new or updated NextHop and a bool indicating whether a new
   *  NextHop was inserted
   */
  std::pair<NextHopList::iterator, bool>
  addOrUpdateNextHop(Face& face, uint64_t cost);

  /** \brief Removes a NextHop record.
   *
   *  If no NextHop record for face exists, do nothing.
   */
  bool
  removeNextHop(const Face& face);

  /** \note This method is non-const because mutable iterators are needed by callers.
   */
  NextHopList::iterator
  findNextHop(const Face& face) noexcept;

  /** \brief Sorts the nexthop list.
   */
  void
  sortNextHops();

private:
  Name m_prefix;
  NextHopList m_nextHops;

  name_tree::Entry* m_nameTreeEntry = nullptr;

  friend ::nfd::name_tree::Entry;
  friend Fib;
};

} // namespace fib
} // namespace nfd

#endif // NFD_DAEMON_TABLE_FIB_ENTRY_HPP
