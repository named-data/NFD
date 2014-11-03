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

#ifndef NFD_DAEMON_TABLE_FIB_ENTRY_HPP
#define NFD_DAEMON_TABLE_FIB_ENTRY_HPP

#include "fib-nexthop.hpp"

namespace nfd {

class NameTree;
namespace name_tree {
class Entry;
}

namespace fib {

/** \class NextHopList
 *  \brief represents a collection of nexthops
 *
 *  This type has these methods as public API:
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

  /** \return whether this Entry has any NextHop record
   */
  bool
  hasNextHops() const;

  /** \return whether there is a NextHop record for face
   *
   *  \todo change parameter type to Face&
   */
  bool
  hasNextHop(shared_ptr<Face> face) const;

  /** \brief adds a NextHop record
   *
   *  If a NextHop record for face already exists, its cost is updated.
   *  \note shared_ptr is passed by value because this function will take shared ownership
   */
  void
  addNextHop(shared_ptr<Face> face, uint64_t cost);

  /** \brief removes a NextHop record
   *
   *  If no NextHop record for face exists, do nothing.
   *
   *  \todo change parameter type to Face&
   */
  void
  removeNextHop(shared_ptr<Face> face);

private:
  /** @note This method is non-const because normal iterator is needed by callers.
   */
  NextHopList::iterator
  findNextHop(Face& face);

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

#endif // NFD_DAEMON_TABLE_FIB_ENTRY_HPP
