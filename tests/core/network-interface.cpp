/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "core/network-interface.hpp"
#include "tests/test-common.hpp"

#include <boost/foreach.hpp>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreNetworkInterface, BaseFixture)

BOOST_AUTO_TEST_CASE(ListNetworkInterfaces)
{
  std::list< shared_ptr<NetworkInterfaceInfo> > netifs;
  BOOST_CHECK_NO_THROW(netifs = listNetworkInterfaces());

  BOOST_FOREACH(shared_ptr<NetworkInterfaceInfo> netif, netifs)
  {
    BOOST_TEST_MESSAGE(netif->index << ": " << netif->name);
    BOOST_TEST_MESSAGE("\tether " << netif->etherAddress);
    BOOST_FOREACH(boost::asio::ip::address_v4 address, netif->ipv4Addresses)
      BOOST_TEST_MESSAGE("\tinet  " << address);
    BOOST_FOREACH(boost::asio::ip::address_v6 address, netif->ipv6Addresses)
      BOOST_TEST_MESSAGE("\tinet6 " << address);
    BOOST_TEST_MESSAGE("\tloopback  : " << netif->isLoopback());
    BOOST_TEST_MESSAGE("\tmulticast : " << netif->isMulticastCapable());
    BOOST_TEST_MESSAGE("\tup        : " << netif->isUp());
  }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
