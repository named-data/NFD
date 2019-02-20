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

#ifndef NFD_TESTS_DAEMON_RIB_CREATE_ROUTE_HPP
#define NFD_TESTS_DAEMON_RIB_CREATE_ROUTE_HPP

#include "rib/route.hpp"

namespace nfd {
namespace rib {
namespace tests {

inline Route
createRoute(uint64_t faceId,
            std::underlying_type_t<ndn::nfd::RouteOrigin> origin,
            uint64_t cost = 0,
            std::underlying_type_t<ndn::nfd::RouteFlags> flags = ndn::nfd::ROUTE_FLAGS_NONE)
{
  Route r;
  r.faceId = faceId;
  r.origin = static_cast<ndn::nfd::RouteOrigin>(origin);
  r.cost = cost;
  r.flags = flags;
  return r;
}

} // namespace tests
} // namespace rib
} // namespace nfd

#endif // NFD_TESTS_DAEMON_RIB_CREATE_ROUTE_HPP
