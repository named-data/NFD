/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

namespace nfd::rib {

Route::Route() = default;

static time::steady_clock::time_point
computeExpiration(const ndn::PrefixAnnouncement& ann)
{
  auto validityEnd = time::steady_clock::duration::max();
  const auto& validityPeriod = ann.getValidityPeriod();
  if (validityPeriod) {
    auto now = time::system_clock::now();
    if (!validityPeriod->isValid(now)) {
      validityEnd = time::steady_clock::duration::zero();
    }
    else {
      validityEnd = validityPeriod->getPeriod().second - now;
    }
  }
  return time::steady_clock::now() +
    std::min(validityEnd, time::duration_cast<time::steady_clock::duration>(ann.getExpiration()));
}

Route::Route(const ndn::PrefixAnnouncement& ann, uint64_t faceId)
  : faceId(faceId)
  , origin(ndn::nfd::ROUTE_ORIGIN_PREFIXANN)
  , cost(PA_ROUTE_COST)
  , flags(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT)
  , expires(computeExpiration(ann))
  , announcement(ann)
  , annExpires(*expires)
{
}

std::ostream&
operator<<(std::ostream& os, const Route& route)
{
  os << "Route{"
     << "face: " << route.faceId
     << ", origin: " << route.origin
     << ", cost: " << route.cost
     << ", flags: " << ndn::AsHex{route.flags};

  if (route.expires) {
    os << ", expires in: " << time::duration_cast<time::milliseconds>(*route.expires - time::steady_clock::now());
  }
  else {
    os << ", expires: never";
  }

  if (route.announcement) {
    os << ", announcement: (" << *route.announcement << ')';
  }

  return os << '}';
}

} // namespace nfd::rib
