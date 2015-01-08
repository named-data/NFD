/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_CS_ENTRY_HPP
#define NFD_DAEMON_TABLE_CS_ENTRY_HPP

#include "cs.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace cs {

/** \brief an Entry in ContentStore
 *  \note This type is internal to ContentStore.
 *
 *  An Entry is either a stored Entry which contains a Data packet and related attributes,
 *  or a query Entry which contains a Name that is LessComparable to other stored/query Entry
 *  and is used to lookup a container of entries.
 */
class Entry
{
public:
  /** \brief construct Entry for query
   *  \note Name is implicitly convertible to Entry, so that Name can be passed to
   *        lookup functions on a container of Entry
   */
  Entry(const Name& name);

  /** \brief construct Entry for storage
   */
  Entry(shared_ptr<const Data> data, bool isUnsolicited);

  shared_ptr<const Data>
  getData() const;

  const Name&
  getName() const;

  const Name&
  getFullName() const;

  /** \return true if entry can become stale, false if entry is never stale
   */
  bool
  canStale() const;

  bool
  isStale() const;

  void
  refresh();

  bool
  isUnsolicited() const;

  void
  unsetUnsolicited();

  /** \brief determines whether Interest can be satisified by this cached Data
   *  \note ChildSelector is not considered
   */
  bool
  canSatisfy(const Interest& interest) const;

  bool
  operator<(const Entry& other) const;

private:
  bool
  isQuery() const;

public:
  Cs::QueueType queueType;
  Cs::QueueIt queueIt;
  scheduler::EventId moveStaleEvent;

private:
  Name m_queryName;
  shared_ptr<const Data> m_data;
  time::steady_clock::TimePoint m_staleAt;
  bool m_isUnsolicited;
};

} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_ENTRY_HPP
