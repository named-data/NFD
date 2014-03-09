/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_NETWORK_INTERFACE_HPP
#define NFD_CORE_NETWORK_INTERFACE_HPP

#include "common.hpp"
#include "face/ethernet.hpp"

#include <net/if.h>

namespace nfd {

class NetworkInterfaceInfo
{
public:
  int index;
  std::string name;
  ethernet::Address etherAddress;
  std::vector<boost::asio::ip::address_v4> ipv4Addresses;
  std::vector<boost::asio::ip::address_v6> ipv6Addresses;
  unsigned int flags;

  bool
  isLoopback() const;

  bool
  isMulticastCapable() const;

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
NetworkInterfaceInfo::isUp() const
{
  return (flags & IFF_UP) != 0;
}

std::list< shared_ptr<NetworkInterfaceInfo> >
listNetworkInterfaces();

} // namespace nfd

#endif // NFD_CORE_NETWORK_INTERFACE_HPP
