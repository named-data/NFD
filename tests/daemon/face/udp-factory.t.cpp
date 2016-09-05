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

#include "face/udp-factory.hpp"

#include "factory-test-common.hpp"
#include "core/network-interface.hpp"
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

BOOST_AUTO_TEST_CASE(CreateChannel)
{
  UdpFactory factory;

  auto channel1 = factory.createChannel("127.0.0.1", "20070");
  auto channel1a = factory.createChannel("127.0.0.1", "20070");
  BOOST_CHECK_EQUAL(channel1, channel1a);
  BOOST_CHECK_EQUAL(channel1->getUri().toString(), "udp4://127.0.0.1:20070");

  auto channel2 = factory.createChannel("127.0.0.1", "20071");
  BOOST_CHECK_NE(channel1, channel2);

  auto channel3 = factory.createChannel("::1", "20071");
  BOOST_CHECK_NE(channel2, channel3);
  BOOST_CHECK_EQUAL(channel3->getUri().toString(), "udp6://[::1]:20071");

  // createChannel with multicast address
  BOOST_CHECK_EXCEPTION(factory.createChannel("224.0.0.1", "20070"), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "createChannel is only for unicast channels. The provided endpoint "
                                        "is multicast. Use createMulticastFace to create a multicast face") == 0;
                        });

  // createChannel with a local endpoint that has already been allocated for a UDP multicast face
  auto multicastFace = factory.createMulticastFace("127.0.0.1", "224.0.0.1", "20072");
  BOOST_CHECK_EXCEPTION(factory.createChannel("127.0.0.1", "20072"), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create the requested UDP unicast channel, local "
                                        "endpoint is already allocated for a UDP multicast face") == 0;
                        });
}

BOOST_AUTO_TEST_CASE(CreateMulticastFace)
{
  UdpFactory factory;

  auto multicastFace1  = factory.createMulticastFace("127.0.0.1", "224.0.0.1", "20070");
  auto multicastFace1a = factory.createMulticastFace("127.0.0.1", "224.0.0.1", "20070");
  BOOST_CHECK_EQUAL(multicastFace1, multicastFace1a);

  // createMulticastFace with a local endpoint that is already used by a channel
  auto channel = factory.createChannel("127.0.0.1", "20071");
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace("127.0.0.1", "224.0.0.1", "20071"), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create the requested UDP multicast face, local "
                                        "endpoint is already allocated for a UDP unicast channel") == 0;
                        });

  // createMulticastFace with a local endpoint that is already
  // used by a multicast face on a different multicast group
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace("127.0.0.1", "224.0.0.42", "20070"), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create the requested UDP multicast face, local "
                                        "endpoint is already allocated for a UDP multicast face "
                                        "on a different multicast group") == 0;
                        });

  // createMulticastFace with an IPv4 unicast address
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace("127.0.0.1", "192.168.10.15", "20072"), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create the requested UDP multicast face, "
                                        "the multicast group given as input is not a multicast address") == 0;
                        });

  // createMulticastFace with an IPv6 multicast address
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace("::1", "ff01::114", "20073"), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "IPv6 multicast is not supported yet. Please provide an IPv4 "
                                        "address") == 0;
                        });

  // createMulticastFace with different local and remote port numbers
  udp::Endpoint localEndpoint(boost::asio::ip::address_v4::loopback(), 20074);
  udp::Endpoint multicastEndpoint(boost::asio::ip::address::from_string("224.0.0.1"), 20075);
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(localEndpoint, multicastEndpoint), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create the requested UDP multicast face, "
                                        "both endpoints should have the same port number. ") == 0;
                        });
}

BOOST_AUTO_TEST_CASE(FaceCreate)
{
  UdpFactory factory = UdpFactory();

  createFace(factory,
             FaceUri("udp4://127.0.0.1:6363"),
             ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             false,
             {CreateFaceExpectedResult::FAILURE, 504, "No channels available to connect"});

  factory.createChannel("127.0.0.1", "20071");

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20070"),
             ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             false,
             {CreateFaceExpectedResult::SUCCESS, 0, ""});
  //test the upgrade
  createFace(factory,
             FaceUri("udp4://127.0.0.1:20070"),
             ndn::nfd::FACE_PERSISTENCY_PERMANENT,
             false,
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20072"),
             ndn::nfd::FACE_PERSISTENCY_PERMANENT,
             false,
             {CreateFaceExpectedResult::SUCCESS, 0, ""});
}

BOOST_AUTO_TEST_CASE(UnsupportedFaceCreate)
{
  UdpFactory factory = UdpFactory();

  factory.createChannel("127.0.0.1", "20070");

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20070"),
             ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
             false,
             {CreateFaceExpectedResult::FAILURE, 406,
               "Outgoing unicast UDP faces do not support on-demand persistency"});
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
