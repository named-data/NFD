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

#include "common.hpp"

namespace nfd {
namespace cs {

/** \brief represents a base class for CS entry
 */
class Entry
{
public: // exposed through ContentStore enumeration
  /** \return the stored Data
   *  \pre hasData()
   */
  const Data&
  getData() const
  {
    BOOST_ASSERT(this->hasData());
    return *m_data;
  }

  /** \return Name of the stored Data
   *  \pre hasData()
   */
  const Name&
  getName() const
  {
    BOOST_ASSERT(this->hasData());
    return m_data->getName();
  }

  /** \return full name (including implicit digest) of the stored Data
   *  \pre hasData()
   */
  const Name&
  getFullName() const
  {
    BOOST_ASSERT(this->hasData());
    return m_data->getFullName();
  }

  /** \return whether the stored Data is unsolicited
   *  \pre hasData()
   */
  bool
  isUnsolicited() const
  {
    BOOST_ASSERT(this->hasData());
    return m_isUnsolicited;
  }

  /** \return the absolute time when the stored Data becomes expired
   *  \retval time::steady_clock::TimePoint::max() if the stored Data never expires
   *  \pre hasData()
   */
  const time::steady_clock::TimePoint&
  getStaleTime() const
  {
    BOOST_ASSERT(this->hasData());
    return m_staleTime;
  }

  /** \brief checks if the stored Data is stale now
   *  \pre hasData()
   */
  bool
  isStale() const;

  /** \brief determines whether Interest can be satisified by the stored Data
   *  \note ChildSelector is not considered
   *  \pre hasData()
   */
  bool
  canSatisfy(const Interest& interest) const;

public: // used by generic ContentStore implementation
  /** \return true if a Data packet is stored
   */
  bool
  hasData() const
  {
    return m_data != nullptr;
  }

  /** \brief replaces the stored Data
   */
  void
  setData(shared_ptr<const Data> data, bool isUnsolicited);

  /** \brief replaces the stored Data
   */
  void
  setData(const Data& data, bool isUnsolicited)
  {
    this->setData(data.shared_from_this(), isUnsolicited);
  }

  /** \brief refreshes stale time relative to current time
   */
  void
  updateStaleTime();

  /** \brief clears the entry
   *  \post !hasData()
   */
  void
  reset();

private:
  shared_ptr<const Data> m_data;
  bool m_isUnsolicited;
  time::steady_clock::TimePoint m_staleTime;
};

} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_ENTRY_HPP
