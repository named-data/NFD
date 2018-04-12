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

#ifndef NFD_RIB_ROUTE_HPP
#define NFD_RIB_ROUTE_HPP

#include "core/scheduler.hpp"

#include <ndn-cxx/encoding/nfd-constants.hpp>
#include <ndn-cxx/mgmt/nfd/route-flags-traits.hpp>

#include <type_traits>

namespace nfd {
namespace rib {

/** \brief represents a route for a name prefix
 */
class Route : public ndn::nfd::RouteFlagsTraits<Route>
{
public:
  Route();

  void
  setExpirationEvent(const scheduler::EventId eid)
  {
    m_expirationEvent = eid;
  }

  const scheduler::EventId&
  getExpirationEvent() const
  {
    return m_expirationEvent;
  }

  std::underlying_type<ndn::nfd::RouteFlags>::type
  getFlags() const
  {
    return flags;
  }

public:
  uint64_t faceId;
  ndn::nfd::RouteOrigin origin;
  uint64_t cost;
  std::underlying_type<ndn::nfd::RouteFlags>::type flags;
  optional<time::steady_clock::TimePoint> expires;

private:
  scheduler::EventId m_expirationEvent;
};

bool
operator==(const Route& lhs, const Route& rhs);

inline bool
operator!=(const Route& lhs, const Route& rhs)
{
  return !(lhs == rhs);
}

inline bool
compareFaceIdAndOrigin(const Route& lhs, const Route& rhs)
{
  return (lhs.faceId == rhs.faceId && lhs.origin == rhs.origin);
}

inline bool
compareFaceId(const Route& route, const uint64_t faceId)
{
  return (route.faceId == faceId);
}

std::ostream&
operator<<(std::ostream& os, const Route& route);

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_ROUTE_HPP
