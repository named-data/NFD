/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "route.hpp"
#include <ndn-cxx/util/string-helper.hpp>

namespace nfd {
namespace rib {

Route::Route()
  : faceId(0)
  , origin(ndn::nfd::ROUTE_ORIGIN_APP)
  , cost(0)
  , flags(ndn::nfd::ROUTE_FLAGS_NONE)
{
}

bool
operator==(const Route& lhs, const Route& rhs)
{
  return lhs.faceId == rhs.faceId &&
         lhs.origin == rhs.origin &&
         lhs.flags == rhs.flags &&
         lhs.cost == rhs.cost &&
         lhs.expires == rhs.expires;
}

std::ostream&
operator<<(std::ostream& os, const Route& route)
{
  os << "Route("
     << "faceid: " << route.faceId
     << ", origin: " << route.origin
     << ", cost: " << route.cost
     << ", flags: " << ndn::AsHex{route.flags};
  if (route.expires) {
    os << ", expires in: " << time::duration_cast<time::milliseconds>(*route.expires - time::steady_clock::now());
  }
  else {
    os << ", never expires";
  }
  os << ")";

  return os;
}

} // namespace rib
} // namespace nfd
