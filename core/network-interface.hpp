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

#ifndef NFD_CORE_NETWORK_INTERFACE_HPP
#define NFD_CORE_NETWORK_INTERFACE_HPP

#include "common.hpp"

#include <ndn-cxx/util/ethernet.hpp>

#include <net/if.h>

namespace nfd {

namespace ethernet = ndn::util::ethernet;

/** \brief contains information about a network interface
 */
class NetworkInterfaceInfo
{
public:

  int index;
  std::string name;
  ethernet::Address etherAddress;
  std::vector<boost::asio::ip::address_v4> ipv4Addresses;
  std::vector<boost::asio::ip::address_v6> ipv6Addresses;
  boost::asio::ip::address_v4 broadcastAddress;
  unsigned int flags;

  bool
  isLoopback() const;

  bool
  isMulticastCapable() const;

  bool
  isBroadcastCapable() const;

  bool
  isUp() const;

};

inline bool
NetworkInterfaceInfo::isLoopback() const
{
  return (flags & IFF_LOOPBACK) != 0;
}

inline bool
NetworkInterfaceInfo::isMulticastCapable() const
{
  return (flags & IFF_MULTICAST) != 0;
}

inline bool
NetworkInterfaceInfo::isBroadcastCapable() const
{
  return (flags & IFF_BROADCAST) != 0;
}

inline bool
NetworkInterfaceInfo::isUp() const
{
  return (flags & IFF_UP) != 0;
}

/** \brief List configured network interfaces on the system and their info
 *  \warning invalid IP addresses (e.g., 0.0.0.0) may be returned in some environments
 */
std::vector<NetworkInterfaceInfo>
listNetworkInterfaces();

#ifdef WITH_TESTS
/** \brief Set a list of network interfaces to be returned by subsequent listNetworkInterfaces call
 *  \note To reset to normal behavior, use `setDebugNetworkInterfaces(nullptr);`
 */
void
setDebugNetworkInterfaces(shared_ptr<std::vector<NetworkInterfaceInfo>> interfaces);
#endif

} // namespace nfd

#endif // NFD_CORE_NETWORK_INTERFACE_HPP
