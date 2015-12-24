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

#include "face/udp-factory.hpp"

#include "core/network-interface.hpp"
#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestUdpFactory, BaseFixture)

using nfd::Face;

BOOST_AUTO_TEST_CASE(GetChannels)
{
  UdpFactory factory;
  BOOST_REQUIRE_EQUAL(factory.getChannels().empty(), true);

  std::vector<shared_ptr<const Channel>> expectedChannels;
  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20070"));
  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20071"));
  expectedChannels.push_back(factory.createChannel("::1", "20071"));

  for (const auto& i : factory.getChannels()) {
    auto pos = std::find(expectedChannels.begin(), expectedChannels.end(), i);
    BOOST_REQUIRE(pos != expectedChannels.end());
    expectedChannels.erase(pos);
  }

  BOOST_CHECK_EQUAL(expectedChannels.size(), 0);
}

class FactoryErrorCheck : public BaseFixture
{
public:
  bool
  isTheSameMulticastEndpoint(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP unicast channel, local "
                  "endpoint is already allocated for a UDP multicast face") == 0;
  }

  bool
  isNotMulticastAddress(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP multicast face, "
                  "the multicast group given as input is not a multicast address") == 0;
  }

  bool
  isTheSameUnicastEndpoint(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP multicast face, local "
                  "endpoint is already allocated for a UDP unicast channel") == 0;
  }

  bool
  isLocalEndpointOnDifferentGroup(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP multicast face, local "
                  "endpoint is already allocated for a UDP multicast face "
                  "on a different multicast group") == 0;
  }
};

BOOST_FIXTURE_TEST_CASE(ChannelMapUdp, FactoryErrorCheck)
{
  using boost::asio::ip::udp;

  UdpFactory factory = UdpFactory();

  //to instantiate multicast face on a specific ip address, change interfaceIp
  std::string interfaceIp = "0.0.0.0";

  shared_ptr<UdpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<UdpChannel> channel1a = factory.createChannel("127.0.0.1", "20070");
  BOOST_CHECK_EQUAL(channel1, channel1a);
  BOOST_CHECK_EQUAL(channel1->getUri().toString(), "udp4://127.0.0.1:20070");

  shared_ptr<UdpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");
  BOOST_CHECK_NE(channel1, channel2);

  shared_ptr<UdpChannel> channel3 = factory.createChannel(interfaceIp, "20070");

  shared_ptr<UdpChannel> channel4 = factory.createChannel("::1", "20071");
  BOOST_CHECK_NE(channel2, channel4);
  BOOST_CHECK_EQUAL(channel4->getUri().toString(), "udp6://[::1]:20071");

  //same endpoint of a unicast channel
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp, "224.0.0.1", "20070"),
                        UdpFactory::Error, isTheSameUnicastEndpoint);

  auto multicastFace1  = factory.createMulticastFace(interfaceIp, "224.0.0.1", "20072");
  auto multicastFace1a = factory.createMulticastFace(interfaceIp, "224.0.0.1", "20072");
  BOOST_CHECK_EQUAL(multicastFace1, multicastFace1a);

  //same endpoint of a multicast face
  BOOST_CHECK_EXCEPTION(factory.createChannel(interfaceIp, "20072"),
                        UdpFactory::Error, isTheSameMulticastEndpoint);

  //same multicast endpoint, different group
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp, "224.0.0.42", "20072"),
                        UdpFactory::Error, isLocalEndpointOnDifferentGroup);

  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp, "192.168.10.15", "20025"),
                        UdpFactory::Error, isNotMulticastAddress);


//  //Test commented because it required to be run in a machine that can resolve ipv6 query
//  shared_ptr<UdpChannel> channel1v6 = factory.createChannel(//"::1",
//                                                     "fe80::5e96:9dff:fe7d:9c8d%en1",
//                                                     //"fe80::aa54:b2ff:fe08:27b8%wlan0",
//                                                     "20070");
//
//  //the creation of multicastFace2 works properly. It has been disable because it needs an IP address of
//  //an available network interface (different from the first one used)
//  shared_ptr<MulticastUdpFace> multicastFace2 = factory.createMulticastFace("192.168.1.17",
//                                                                            "224.0.0.1",
//                                                                            "20073");
//  BOOST_CHECK_NE(multicastFace1, multicastFace2);
//
//
//  //ipv6 - work in progress
//  shared_ptr<MulticastUdpFace> multicastFace3 = factory.createMulticastFace("fe80::5e96:9dff:fe7d:9c8d%en1",
//                                                                            "FF01:0:0:0:0:0:0:2",
//                                                                            "20073");
//
//  shared_ptr<MulticastUdpFace> multicastFace4 = factory.createMulticastFace("fe80::aa54:b2ff:fe08:27b8%wlan0",
//                                                                            "FF01:0:0:0:0:0:0:2",
//                                                                            "20073");
//
//  BOOST_CHECK_EQUAL(multicastFace3, multicastFace4);
//
//  shared_ptr<MulticastUdpFace> multicastFace5 = factory.createMulticastFace("::1",
//                                                                            "FF01:0:0:0:0:0:0:2",
//                                                                            "20073");
//
//  BOOST_CHECK_NE(multicastFace3, multicastFace5);
//
//  //same local ipv6 endpoint for a different multicast group
//  BOOST_CHECK_THROW(factory.createMulticastFace("fe80::aa54:b2ff:fe08:27b8%wlan0",
//                                                "FE01:0:0:0:0:0:0:2",
//                                                "20073"),
//                    UdpFactory::Error);
//
//  //same local ipv6 (expect for th port number) endpoint for a different multicast group
//  BOOST_CHECK_THROW(factory.createMulticastFace("fe80::aa54:b2ff:fe08:27b8%wlan0",
//                                                "FE01:0:0:0:0:0:0:2",
//                                                "20075"),
//                    UdpFactory::Error);
//
//  BOOST_CHECK_THROW(factory.createMulticastFace("fa80::20a:9dff:fef6:12ff",
//                                                "FE12:0:0:0:0:0:0:2",
//                                                "20075"),
//                    UdpFactory::Error);
//
//  //not a multicast ipv6
//  BOOST_CHECK_THROW(factory.createMulticastFace("fa80::20a:9dff:fef6:12ff",
//                                                "A112:0:0:0:0:0:0:2",
//                                                "20075"),
//                    UdpFactory::Error);
}

class FaceCreateFixture : protected BaseFixture
{
public:
  void
  checkError(const std::string& errorActual, const std::string& errorExpected)
  {
    BOOST_CHECK_EQUAL(errorActual, errorExpected);
  }

  void
  failIfError(const std::string& errorActual)
  {
    BOOST_FAIL("No error expected, but got: [" << errorActual << "]");
  }
};

BOOST_FIXTURE_TEST_CASE(FaceCreate, FaceCreateFixture)
{
  UdpFactory factory = UdpFactory();

  factory.createFace(FaceUri("udp4://127.0.0.1:6363"),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::checkError, this, _1,
                          "No channels available to connect to 127.0.0.1:6363"));

  factory.createChannel("127.0.0.1", "20071");

  factory.createFace(FaceUri("udp4://127.0.0.1:20070"),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::failIfError, this, _1));
  //test the upgrade
  factory.createFace(FaceUri("udp4://127.0.0.1:20070"),
                     ndn::nfd::FACE_PERSISTENCY_PERMANENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::failIfError, this, _1));

  factory.createFace(FaceUri("udp4://127.0.0.1:20072"),
                     ndn::nfd::FACE_PERSISTENCY_PERMANENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::failIfError, this, _1));
}

BOOST_AUTO_TEST_CASE(UnsupportedFaceCreate)
{
  UdpFactory factory;

  factory.createChannel("127.0.0.1", "20070");

  BOOST_CHECK_THROW(factory.createFace(FaceUri("udp4://127.0.0.1:20070"),
                                       ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);
}

class FakeNetworkInterfaceFixture : public BaseFixture
{
public:
  FakeNetworkInterfaceFixture()
  {
    using namespace boost::asio::ip;

    auto fakeInterfaces = make_shared<std::vector<NetworkInterfaceInfo>>();

    fakeInterfaces->push_back(
      NetworkInterfaceInfo {0, "eth0",
        ethernet::Address::fromString("3e:15:c2:8b:65:00"),
        {address_v4::from_string("0.0.0.0")},
        {address_v6::from_string("::")},
        address_v4(),
        IFF_UP});
    fakeInterfaces->push_back(
      NetworkInterfaceInfo {1, "eth0",
        ethernet::Address::fromString("3e:15:c2:8b:65:00"),
        {address_v4::from_string("192.168.2.1"), address_v4::from_string("192.168.2.2")},
        {},
        address_v4::from_string("192.168.2.255"),
        0});
    fakeInterfaces->push_back(
      NetworkInterfaceInfo {2, "eth1",
        ethernet::Address::fromString("3e:15:c2:8b:65:00"),
        {address_v4::from_string("198.51.100.1")},
        {address_v6::from_string("2001:db8::2"), address_v6::from_string("2001:db8::3")},
        address_v4::from_string("198.51.100.255"),
        IFF_MULTICAST | IFF_BROADCAST | IFF_UP});

    setDebugNetworkInterfaces(fakeInterfaces);
  }

  ~FakeNetworkInterfaceFixture()
  {
    setDebugNetworkInterfaces(nullptr);
  }
};

BOOST_FIXTURE_TEST_CASE(Bug2292, FakeNetworkInterfaceFixture)
{
  using namespace boost::asio::ip;

  UdpFactory factory;
  factory.prohibitEndpoint(udp::Endpoint(address_v4::from_string("192.168.2.1"), 1024));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 1);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<udp::Endpoint> {
                 udp::Endpoint(address_v4::from_string("192.168.2.1"), 1024),
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(udp::Endpoint(address_v6::from_string("2001:db8::1"), 2048));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 1);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<udp::Endpoint> {
                 udp::Endpoint(address_v6::from_string("2001:db8::1"), 2048),
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(udp::Endpoint(address_v4(), 1024));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 6);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<udp::Endpoint> {
                 udp::Endpoint(address_v4::from_string("192.168.2.1"), 1024),
                 udp::Endpoint(address_v4::from_string("192.168.2.2"), 1024),
                 udp::Endpoint(address_v4::from_string("198.51.100.1"), 1024),
                 udp::Endpoint(address_v4::from_string("198.51.100.255"), 1024),
                 udp::Endpoint(address_v4::from_string("255.255.255.255"), 1024),
                 udp::Endpoint(address_v4::from_string("0.0.0.0"), 1024)
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(udp::Endpoint(address_v6(), 2048));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 3);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<udp::Endpoint> {
                 udp::Endpoint(address_v6::from_string("2001:db8::2"), 2048),
                 udp::Endpoint(address_v6::from_string("2001:db8::3"), 2048),
                 udp::Endpoint(address_v6::from_string("::"), 2048),
               }));
}

BOOST_AUTO_TEST_SUITE_END() // TestUdpFactory
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace nfd
