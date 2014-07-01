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

#ifndef NFD_RIB_FACE_ENTRY_HPP
#define NFD_RIB_FACE_ENTRY_HPP

#include "common.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace rib {

/** \class FaceEntry
 *  \brief represents a route for a name prefix
 */
class FaceEntry
{
public:
  FaceEntry()
    : faceId(0)
    , origin(0)
    , flags(0)
    , cost(0)
    , expires(time::steady_clock::TimePoint::min())
    , m_expirationEvent()
  {
  }

public:
  void
  setExpirationEvent(const EventId eid)
  {
    m_expirationEvent = eid;
  }

  const EventId&
  getExpirationEvent() const
  {
    return m_expirationEvent;
  }

public:
  uint64_t faceId;
  uint64_t origin;
  uint64_t flags;
  uint64_t cost;
  time::steady_clock::TimePoint expires;

private:
  EventId m_expirationEvent;
};

inline bool
compareFaceIdAndOrigin(const FaceEntry& entry1, const FaceEntry& entry2)
{
  return (entry1.faceId == entry2.faceId && entry1.origin == entry2.origin);
}

inline bool
compareFaceId(const FaceEntry& entry, const uint64_t faceId)
{
  return (entry.faceId == faceId);
}

// Method definition in rib-entry.cpp
std::ostream&
operator<<(std::ostream& os, const FaceEntry& entry);

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_ENTRY_HPP
