/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_PIT_FACE_RECORD_HPP
#define NFD_DAEMON_TABLE_PIT_FACE_RECORD_HPP

#include "face/face.hpp"
#include "strategy-info-host.hpp"

namespace nfd {
namespace pit {

/** \brief contains information about an Interest
 *         on an incoming or outgoing face
 *  \note This is an implementation detail to extract common functionality
 *        of InRecord and OutRecord
 */
class FaceRecord : public StrategyInfoHost
{
public:
  explicit
  FaceRecord(Face& face);

  Face&
  getFace() const;

  uint32_t
  getLastNonce() const;

  time::steady_clock::TimePoint
  getLastRenewed() const;

  /** \brief gives the time point this record expires
   *  \return getLastRenewed() + InterestLifetime
   */
  time::steady_clock::TimePoint
  getExpiry() const;

  /** \brief updates lastNonce, lastRenewed, expiry fields
   */
  void
  update(const Interest& interest);

private:
  Face& m_face;
  uint32_t m_lastNonce;
  time::steady_clock::TimePoint m_lastRenewed;
  time::steady_clock::TimePoint m_expiry;
};

inline Face&
FaceRecord::getFace() const
{
  return m_face;
}

inline uint32_t
FaceRecord::getLastNonce() const
{
  return m_lastNonce;
}

inline time::steady_clock::TimePoint
FaceRecord::getLastRenewed() const
{
  return m_lastRenewed;
}

inline time::steady_clock::TimePoint
FaceRecord::getExpiry() const
{
  return m_expiry;
}

} // namespace pit
} // namespace nfd

#endif // NFD_DAEMON_TABLE_PIT_FACE_RECORD_HPP
