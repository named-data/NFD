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

#include "udp-protocol.hpp"

namespace nfd {
namespace udp {

ssize_t
computeMtu(const Endpoint& localEndpoint)
{
  size_t mtu = 0;
  if (localEndpoint.address().is_v4()) { // IPv4
    mtu = std::numeric_limits<uint16_t>::max(); // maximum Total Length
    mtu -= sizeof(uint32_t) * ((1 << 4) - 1); // maximum Internet Header Length
  }
  else { // IPv6
    mtu = std::numeric_limits<uint16_t>::max(); // maximum Payload Length
  }
  mtu -= sizeof(uint16_t) * 4; // size of UDP header
  return mtu;
}

} // namespace udp
} // namespace nfd
