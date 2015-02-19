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

#include "core/network-interface.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestNetworkInterface, BaseFixture)

BOOST_AUTO_TEST_CASE(ListRealNetworkInterfaces)
{
  std::vector<NetworkInterfaceInfo> netifs;
  BOOST_CHECK_NO_THROW(netifs = listNetworkInterfaces());

  for (const auto& netif : netifs) {
    BOOST_TEST_MESSAGE(netif.index << ": " << netif.name);
    BOOST_TEST_MESSAGE("\tether " << netif.etherAddress);
    for (const auto& address : netif.ipv4Addresses)
      BOOST_TEST_MESSAGE("\tinet  " << address);
    for (const auto& address : netif.ipv6Addresses)
      BOOST_TEST_MESSAGE("\tinet6 " << address);
    BOOST_TEST_MESSAGE("\tloopback  : " << netif.isLoopback());
    BOOST_TEST_MESSAGE("\tmulticast : " << netif.isMulticastCapable());
    BOOST_TEST_MESSAGE("\tup        : " << netif.isUp());
  }
}

class FakeNetworkInterfaceFixture : public BaseFixture
{
public:
  FakeNetworkInterfaceFixture()
  {
    using namespace boost::asio::ip;

    auto fakeInterfaces = make_shared<std::vector<NetworkInterfaceInfo>>();

    fakeInterfaces->push_back(
      NetworkInterfaceInfo {0, "lo0",
        ethernet::Address(),
        {address_v4::from_string("127.0.0.1")},
        {address_v6::from_string("fe80::1")},
        address_v4::from_string("127.255.255.255"),
        IFF_LOOPBACK | IFF_UP});
    fakeInterfaces->push_back(
      NetworkInterfaceInfo {1, "eth0",
        ethernet::Address::fromString("3e:15:c2:8b:65:00"),
        {address_v4::from_string("192.168.2.1")},
        {},
        address_v4::from_string("192.168.2.255"),
        0});
    fakeInterfaces->push_back(
      NetworkInterfaceInfo {2, "eth1",
        ethernet::Address::fromString("3e:15:c2:8b:65:00"),
        {address_v4::from_string("198.51.100.1")},
        {address_v6::from_string("2001:db8::1")},
        address_v4::from_string("198.51.100.255"),
        IFF_MULTICAST | IFF_BROADCAST | IFF_UP});

    setDebugNetworkInterfaces(fakeInterfaces);
  }

  ~FakeNetworkInterfaceFixture()
  {
    setDebugNetworkInterfaces(nullptr);
  }
};

BOOST_FIXTURE_TEST_CASE(ListFakeNetworkInterfaces, FakeNetworkInterfaceFixture)
{
  std::vector<NetworkInterfaceInfo> netifs;
  BOOST_CHECK_NO_THROW(netifs = listNetworkInterfaces());

  BOOST_REQUIRE_EQUAL(netifs.size(), 3);

  BOOST_CHECK_EQUAL(netifs[0].index, 0);
  BOOST_CHECK_EQUAL(netifs[1].index, 1);
  BOOST_CHECK_EQUAL(netifs[2].index, 2);

  BOOST_CHECK_EQUAL(netifs[0].name, "lo0");
  BOOST_CHECK_EQUAL(netifs[1].name, "eth0");
  BOOST_CHECK_EQUAL(netifs[2].name, "eth1");

  // no real value of testing other parameters
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
