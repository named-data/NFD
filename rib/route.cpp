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

#include "route.hpp"

namespace nfd {
namespace rib {

bool
Route::operator==(const Route& other) const
{
  return (this->faceId == other.faceId &&
          this->origin == other.origin &&
          this->flags == other.flags &&
          this->cost == other.cost &&
          this->expires == other.expires);
}

std::ostream&
operator<<(std::ostream& os, const Route& route)
{
  os << "Route("
     << "faceid: " << route.faceId
     << ", origin: " << route.origin
     << ", cost: " << route.cost
     << ", flags: " << route.flags;
  if (route.expires != time::steady_clock::TimePoint::max()) {
    os << ", expires in: " << (route.expires - time::steady_clock::now());
  }
  else {
    os << ", never expires";
  }
  os << ")";

  return os;
}

} // namespace rib
} // namespace nfd
