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

#include "test-ip.hpp"

#include "core/network-interface.hpp"

namespace nfd {
namespace tests {
namespace {

template<typename E>
bool
matchTristate(E e, bool b)
{
  return (e == E::DontCare) ||
         (e == E::Yes && b) ||
         (e == E::No && !b);
}

} // unnamed namespace

template<>
boost::asio::ip::address_v4
getTestIp(LoopbackAddress loopback, MulticastInterface mcast)
{
  for (const auto& interface : listNetworkInterfaces()) {
    if (interface.isUp() &&
        matchTristate(loopback, interface.isLoopback()) &&
        matchTristate(mcast, interface.isMulticastCapable())) {
      for (const auto& address : interface.ipv4Addresses) {
        if (!address.is_unspecified() &&
            matchTristate(loopback, address.is_loopback())) {
          return address;
        }
      }
    }
  }
  return {};
}

template<>
boost::asio::ip::address_v6
getTestIp(LoopbackAddress loopback, MulticastInterface mcast)
{
  for (const auto& interface : listNetworkInterfaces()) {
    if (interface.isUp() &&
        matchTristate(loopback, interface.isLoopback()) &&
        matchTristate(mcast, interface.isMulticastCapable())) {
      for (const auto& address : interface.ipv6Addresses) {
        if (!address.is_unspecified() &&
            !address.is_link_local() && // see #1428
            matchTristate(loopback, address.is_loopback())) {
          return address;
        }
      }
    }
  }
  return {};
}

} // namespace tests
} // namespace nfd
