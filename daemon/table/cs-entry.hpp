/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "core/common.hpp"

namespace nfd {
namespace cs {

/** \brief a ContentStore entry
 */
class Entry
{
public: // exposed through ContentStore enumeration
  /** \brief return the stored Data
   */
  const Data&
  getData() const
  {
    return *m_data;
  }

  /** \brief return stored Data name
   */
  const Name&
  getName() const
  {
    return m_data->getName();
  }

  /** \brief return full name (including implicit digest) of the stored Data
   */
  const Name&
  getFullName() const
  {
    return m_data->getFullName();
  }

  /** \brief return whether the stored Data is unsolicited
   */
  bool
  isUnsolicited() const
  {
    return m_isUnsolicited;
  }

  /** \brief check if the stored Data is fresh now
   */
  bool
  isFresh() const;

  /** \brief determine whether Interest can be satisified by the stored Data
   */
  bool
  canSatisfy(const Interest& interest) const;

public: // used by ContentStore implementation
  Entry(shared_ptr<const Data> data, bool isUnsolicited);

  /** \brief recalculate when the entry would become non-fresh, relative to current time
   */
  void
  updateFreshUntil();

  /** \brief clear 'unsolicited' flag
   */
  void
  clearUnsolicited()
  {
    m_isUnsolicited = false;
  }

private:
  shared_ptr<const Data> m_data;
  bool m_isUnsolicited;
  time::steady_clock::TimePoint m_freshUntil;
};

bool
operator<(const Entry& entry, const Name& queryName);

bool
operator<(const Name& queryName, const Entry& entry);

bool
operator<(const Entry& lhs, const Entry& rhs);

/** \brief an ordered container of ContentStore entries
 *
 *  This container uses std::less<> comparator to enable lookup with queryName.
 */
using Table = std::set<Entry, std::less<>>;

inline bool
operator<(Table::const_iterator lhs, Table::const_iterator rhs)
{
  return *lhs < *rhs;
}

} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_ENTRY_HPP
