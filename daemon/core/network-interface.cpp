/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "network-interface.hpp"
#include "core/logger.hpp"

#include <boost/foreach.hpp>

#include <arpa/inet.h>   // for inet_ntop()
#include <netinet/in.h>  // for struct sockaddr_in{,6}
#include <ifaddrs.h>     // for getifaddrs()

#if defined(__linux__)
#include <net/if_arp.h>        // for ARPHRD_* constants
#include <netpacket/packet.h>  // for struct sockaddr_ll
#elif defined(__APPLE__)
#include <net/if_dl.h>         // for struct sockaddr_dl
#else
#error Platform not supported
#endif

namespace nfd {

NFD_LOG_INIT("NetworkInterfaceInfo")

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
#elif defined(__APPLE__)
        case AF_LINK: {
            const sockaddr_dl* sdl = reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr);
            netif->index = sdl->sdl_index;
            netif->etherAddress = ethernet::Address(reinterpret_cast<uint8_t*>(LLADDR(sdl)));
          }
          break;
#endif
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
