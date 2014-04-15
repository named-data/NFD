/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "network-interface.hpp"
#include "core/logger.hpp"

#include <boost/foreach.hpp>

#include <arpa/inet.h>   // for inet_ntop()
#include <netinet/in.h>  // for struct sockaddr_in{,6}
#include <ifaddrs.h>     // for getifaddrs()

#if defined(__linux__)
#include <net/if_arp.h>        // for ARPHRD_* constants
#include <netpacket/packet.h>  // for struct sockaddr_ll
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_dl.h>         // for struct sockaddr_dl
#else
#error Platform not supported
#endif

NFD_LOG_INIT("NetworkInterfaceInfo");

namespace nfd {

std::list< shared_ptr<NetworkInterfaceInfo> >
listNetworkInterfaces()
{
  typedef std::map< std::string, shared_ptr<NetworkInterfaceInfo> > InterfacesMap;
  InterfacesMap ifmap;

  ifaddrs* ifa_list;
  if (::getifaddrs(&ifa_list) < 0)
    throw std::runtime_error("getifaddrs() failed");

  for (ifaddrs* ifa = ifa_list; ifa != 0; ifa = ifa->ifa_next)
    {
      shared_ptr<NetworkInterfaceInfo> netif;
      std::string ifname(ifa->ifa_name);
      InterfacesMap::iterator i = ifmap.find(ifname);
      if (i == ifmap.end())
        {
          netif = make_shared<NetworkInterfaceInfo>();
          netif->name = ifname;
          netif->flags = ifa->ifa_flags;
          ifmap[ifname] = netif;
        }
      else
        {
          netif = i->second;
        }

      if (!ifa->ifa_addr)
        continue;

      switch (ifa->ifa_addr->sa_family)
        {
        case AF_INET: {
            const sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            char address[INET_ADDRSTRLEN];
            if (::inet_ntop(AF_INET, &sin->sin_addr, address, sizeof(address)))
              netif->ipv4Addresses.push_back(boost::asio::ip::address_v4::from_string(address));
            else
              NFD_LOG_WARN("inet_ntop() failed on " << ifname);
          }
          break;
        case AF_INET6: {
            const sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr);
            char address[INET6_ADDRSTRLEN];
            if (::inet_ntop(AF_INET6, &sin6->sin6_addr, address, sizeof(address)))
              netif->ipv6Addresses.push_back(boost::asio::ip::address_v6::from_string(address));
            else
              NFD_LOG_WARN("inet_ntop() failed on " << ifname);
          }
          break;
#if defined(__linux__)
        case AF_PACKET: {
            const sockaddr_ll* sll = reinterpret_cast<sockaddr_ll*>(ifa->ifa_addr);
            netif->index = sll->sll_ifindex;
            if (sll->sll_hatype == ARPHRD_ETHER && sll->sll_halen == ethernet::ADDR_LEN)
              netif->etherAddress = ethernet::Address(sll->sll_addr);
            else if (sll->sll_hatype != ARPHRD_LOOPBACK)
              NFD_LOG_WARN("Unrecognized hardware address on " << ifname);
          }
          break;
#elif defined(__APPLE__) || defined(__FreeBSD__)
        case AF_LINK: {
            const sockaddr_dl* sdl = reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr);
            netif->index = sdl->sdl_index;
            netif->etherAddress = ethernet::Address(reinterpret_cast<uint8_t*>(LLADDR(sdl)));
          }
          break;
#endif
        }

      if (netif->isBroadcastCapable() && ifa->ifa_broadaddr != 0)
        {
          const sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_broadaddr);

          char address[INET_ADDRSTRLEN];
          if (::inet_ntop(AF_INET, &sin->sin_addr, address, sizeof(address)))
            netif->broadcastAddress = boost::asio::ip::address_v4::from_string(address);
          else
            NFD_LOG_WARN("inet_ntop() failed on " << ifname);
        }
    }

  ::freeifaddrs(ifa_list);

  std::list< shared_ptr<NetworkInterfaceInfo> > list;
  BOOST_FOREACH(InterfacesMap::value_type elem, ifmap) {
    list.push_back(elem.second);
  }

  return list;
}

} // namespace nfd
