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

#include "network-interface.hpp"
#include "core/logger.hpp"

#include <cerrno>
#include <cstring>
#include <type_traits>
#include <unordered_map>

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>     // for getifaddrs()

#include <arpa/inet.h>   // for inet_ntop()
#include <netinet/in.h>  // for struct sockaddr_in{,6}

#if defined(__linux__)
#include <net/if_arp.h>        // for ARPHRD_* constants
#include <netpacket/packet.h>  // for struct sockaddr_ll
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_dl.h>         // for struct sockaddr_dl
#include <net/if_types.h>      // for IFT_* constants
#endif

#endif // HAVE_IFADDRS_H


NFD_LOG_INIT("NetworkInterfaceInfo");

namespace nfd {

static_assert(std::is_standard_layout<NetworkInterfaceInfo>::value,
              "NetworkInterfaceInfo must be a standard layout type");
#ifdef HAVE_IS_DEFAULT_CONSTRUCTIBLE
static_assert(std::is_default_constructible<NetworkInterfaceInfo>::value,
              "NetworkInterfaceInfo must provide a default constructor");
#endif

#ifdef WITH_TESTS
static shared_ptr<std::vector<NetworkInterfaceInfo>> s_debugNetworkInterfaces = nullptr;

void
setDebugNetworkInterfaces(shared_ptr<std::vector<NetworkInterfaceInfo>> interfaces)
{
  s_debugNetworkInterfaces = interfaces;
}
#endif

std::vector<NetworkInterfaceInfo>
listNetworkInterfaces()
{
#ifdef WITH_TESTS
  if (s_debugNetworkInterfaces != nullptr) {
    return *s_debugNetworkInterfaces;
  }
#endif

#ifdef HAVE_IFADDRS_H
  using namespace boost::asio::ip;
  using std::strerror;

  std::unordered_map<std::string, NetworkInterfaceInfo> ifmap;
  ifaddrs* ifa_list = nullptr;

  if (::getifaddrs(&ifa_list) < 0)
    BOOST_THROW_EXCEPTION(std::runtime_error(std::string("getifaddrs() failed: ") +
                                             strerror(errno)));

  for (ifaddrs* ifa = ifa_list; ifa != nullptr; ifa = ifa->ifa_next) {
    std::string ifname(ifa->ifa_name);
    NetworkInterfaceInfo& netif = ifmap[ifname];
    netif.name = ifa->ifa_name;
    netif.flags = ifa->ifa_flags;

    if (ifa->ifa_addr == nullptr)
      continue;

    switch (ifa->ifa_addr->sa_family) {

    case AF_INET: {
      const sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
      char address[INET_ADDRSTRLEN];
      if (::inet_ntop(AF_INET, &sin->sin_addr, address, sizeof(address))) {
        netif.ipv4Addresses.push_back(address_v4::from_string(address));
        NFD_LOG_TRACE(ifname << ": added IPv4 address " << address);
      }
      else
        NFD_LOG_WARN(ifname << ": inet_ntop(AF_INET) failed: " << strerror(errno));
      break;
    }

    case AF_INET6: {
      const sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr);
      char address[INET6_ADDRSTRLEN];
      if (::inet_ntop(AF_INET6, &sin6->sin6_addr, address, sizeof(address))) {
        netif.ipv6Addresses.push_back(address_v6::from_string(address));
        NFD_LOG_TRACE(ifname << ": added IPv6 address " << address);
      }
      else
        NFD_LOG_WARN(ifname << ": inet_ntop(AF_INET6) failed: " << strerror(errno));
      break;
    }

#if defined(__linux__)
    case AF_PACKET: {
      const sockaddr_ll* sll = reinterpret_cast<sockaddr_ll*>(ifa->ifa_addr);
      netif.index = sll->sll_ifindex;
      if (sll->sll_hatype == ARPHRD_ETHER && sll->sll_halen == ethernet::ADDR_LEN) {
        netif.etherAddress = ethernet::Address(sll->sll_addr);
        NFD_LOG_TRACE(ifname << ": added Ethernet address " << netif.etherAddress);
      }
      else if (sll->sll_hatype != ARPHRD_LOOPBACK) {
        NFD_LOG_DEBUG(ifname << ": ignoring link-layer address for unhandled hardware type "
                      << sll->sll_hatype);
      }
      break;
    }

#elif defined(__APPLE__) || defined(__FreeBSD__)
    case AF_LINK: {
      const sockaddr_dl* sdl = reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr);
      netif.index = sdl->sdl_index;
      if (sdl->sdl_type == IFT_ETHER && sdl->sdl_alen == ethernet::ADDR_LEN) {
        netif.etherAddress = ethernet::Address(reinterpret_cast<uint8_t*>(LLADDR(sdl)));
        NFD_LOG_TRACE(ifname << ": added Ethernet address " << netif.etherAddress);
      }
      else if (sdl->sdl_type != IFT_LOOP) {
        NFD_LOG_DEBUG(ifname << ": ignoring link-layer address for unhandled interface type "
                      << sdl->sdl_type);
      }
      break;
    }
#endif
    }

    if (netif.isBroadcastCapable() && ifa->ifa_broadaddr != nullptr) {
      const sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_broadaddr);
      char address[INET_ADDRSTRLEN];
      if (::inet_ntop(AF_INET, &sin->sin_addr, address, sizeof(address))) {
        netif.broadcastAddress = address_v4::from_string(address);
        NFD_LOG_TRACE(ifname << ": added IPv4 broadcast address " << address);
      }
      else
        NFD_LOG_WARN(ifname << ": inet_ntop(AF_INET) for broadaddr failed: " << strerror(errno));
    }
  }

  ::freeifaddrs(ifa_list);

  std::vector<NetworkInterfaceInfo> v;
  v.reserve(ifmap.size());
  for (auto&& element : ifmap) {
    v.push_back(element.second);
  }

  return v;
#else
  return {};
#endif // HAVE_IFADDRS_H
}

} // namespace nfd
