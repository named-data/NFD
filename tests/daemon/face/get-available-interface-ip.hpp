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

#ifndef NFD_TESTS_DAEMON_FACE_GET_AVAILABLE_INTERFACE_IP_HPP
#define NFD_TESTS_DAEMON_FACE_GET_AVAILABLE_INTERFACE_IP_HPP

#include "core/network-interface.hpp"

#define SKIP_IF_IP_UNAVAILABLE(address) \
  do { \
    if (address.is_unspecified()) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require a valid IP address"); \
      return; \
    } \
  } while (false)

namespace nfd {
namespace face {
namespace tests {

/** \brief get a non-local IP address from any available network interface
 *  \tparam A the address type, either boost::asio::ip::address_v4 or boost::asio::ip::address_v6
 *  \param needMulticast true if the address must be chosen from a multicast-capable interface
 *  \return an IP address
 *  \retval default-constructed A, if no address is available
 */
template<typename A>
A
getAvailableInterfaceIp(bool needMulticast = false);

template<>
inline boost::asio::ip::address_v4
getAvailableInterfaceIp(bool needMulticast)
{
  for (const auto& interface : listNetworkInterfaces()) {
    if (interface.isUp() &&
        !interface.isLoopback() &&
        (!needMulticast || interface.isMulticastCapable())) {
      for (const auto& address : interface.ipv4Addresses) {
        if (!address.is_unspecified() &&
            !address.is_loopback()) {
          return address;
        }
      }
    }
  }
  return {};
}

template<>
inline boost::asio::ip::address_v6
getAvailableInterfaceIp(bool needMulticast)
{
  for (const auto& interface : listNetworkInterfaces()) {
    if (interface.isUp() &&
        !interface.isLoopback() &&
        (!needMulticast || interface.isMulticastCapable())) {
      for (const auto& address : interface.ipv6Addresses) {
        if (!address.is_unspecified() &&
            !address.is_link_local() && // see #1428
            !address.is_loopback()) {
          return address;
        }
      }
    }
  }
  return {};
}

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_GET_AVAILABLE_INTERFACE_IP_HPP
