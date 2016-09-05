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

#include "face/tcp-factory.hpp"

#include "core/network-interface.hpp"
#include "factory-test-common.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestTcpFactory, BaseFixture)

using nfd::Face;

BOOST_AUTO_TEST_CASE(ChannelMap)
{
  TcpFactory factory;

  shared_ptr<TcpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<TcpChannel> channel1a = factory.createChannel("127.0.0.1", "20070");
  BOOST_CHECK_EQUAL(channel1, channel1a);
  BOOST_CHECK_EQUAL(channel1->getUri().toString(), "tcp4://127.0.0.1:20070");

  shared_ptr<TcpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");
  BOOST_CHECK_NE(channel1, channel2);

  shared_ptr<TcpChannel> channel3 = factory.createChannel("::1", "20071");
  BOOST_CHECK_NE(channel2, channel3);
  BOOST_CHECK_EQUAL(channel3->getUri().toString(), "tcp6://[::1]:20071");
}

BOOST_AUTO_TEST_CASE(GetChannels)
{
  TcpFactory factory;
  BOOST_REQUIRE_EQUAL(factory.getChannels().empty(), true);

  std::vector<shared_ptr<const Channel>> expectedChannels;
  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20070"));
  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20071"));
  expectedChannels.push_back(factory.createChannel("::1", "20071"));

  for (const auto& ch : factory.getChannels()) {
    auto pos = std::find(expectedChannels.begin(), expectedChannels.end(), ch);
    BOOST_REQUIRE(pos != expectedChannels.end());
    expectedChannels.erase(pos);
  }
  BOOST_CHECK_EQUAL(expectedChannels.size(), 0);
}

BOOST_AUTO_TEST_CASE(FaceCreate)
{
  TcpFactory factory;

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:6363"),
             ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             false,
             {CreateFaceExpectedResult::FAILURE, 504, "No channels available to connect"});

  factory.createChannel("127.0.0.1", "20071");

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:20070"),
             ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             false,
             {CreateFaceExpectedResult::SUCCESS, 0, ""});
}

BOOST_AUTO_TEST_CASE(UnsupportedFaceCreate)
{
  TcpFactory factory;

  factory.createChannel("127.0.0.1", "20070");
  factory.createChannel("127.0.0.1", "20071");

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:20070"),
             ndn::nfd::FACE_PERSISTENCY_PERMANENT,
             false,
             {CreateFaceExpectedResult::FAILURE, 406,
               "Outgoing TCP faces only support persistent persistency"});

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:20071"),
             ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
             false,
             {CreateFaceExpectedResult::FAILURE, 406,
               "Outgoing TCP faces only support persistent persistency"});
}

class FaceCreateTimeoutFixture : public BaseFixture
{
public:
  void
  onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK_MESSAGE(false, "Timeout expected");
    BOOST_CHECK(!static_cast<bool>(face1));
    face1 = newFace;

    limitedIo.afterOp();
  }

  void
  onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(true, reason);

    limitedIo.afterOp();
  }

public:
  LimitedIo limitedIo;

  shared_ptr<Face> face1;
};

BOOST_FIXTURE_TEST_CASE(FaceCreateTimeout, FaceCreateTimeoutFixture)
{
  TcpFactory factory;
  shared_ptr<TcpChannel> channel = factory.createChannel("0.0.0.0", "20070");

  factory.createFace(FaceUri("tcp4://192.0.2.1:20070"),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     false,
                     bind(&FaceCreateTimeoutFixture::onFaceCreated, this, _1),
                     bind(&FaceCreateTimeoutFixture::onConnectFailed, this, _2));

  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");

  BOOST_CHECK_EQUAL(static_cast<bool>(face1), false);
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

  TcpFactory factory;
  factory.prohibitEndpoint(tcp::Endpoint(address_v4::from_string("192.168.2.1"), 1024));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 1);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<tcp::Endpoint> {
                 tcp::Endpoint(address_v4::from_string("192.168.2.1"), 1024),
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(tcp::Endpoint(address_v6::from_string("2001:db8::1"), 2048));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 1);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<tcp::Endpoint> {
                 tcp::Endpoint(address_v6::from_string("2001:db8::1"), 2048)
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(tcp::Endpoint(address_v4(), 1024));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 4);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<tcp::Endpoint> {
                 tcp::Endpoint(address_v4::from_string("192.168.2.1"), 1024),
                 tcp::Endpoint(address_v4::from_string("192.168.2.2"), 1024),
                 tcp::Endpoint(address_v4::from_string("198.51.100.1"), 1024),
                 tcp::Endpoint(address_v4::from_string("0.0.0.0"), 1024)
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(tcp::Endpoint(address_v6(), 2048));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 3);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<tcp::Endpoint> {
                 tcp::Endpoint(address_v6::from_string("2001:db8::2"), 2048),
                 tcp::Endpoint(address_v6::from_string("2001:db8::3"), 2048),
                 tcp::Endpoint(address_v6::from_string("::"), 2048)
               }));
}

BOOST_AUTO_TEST_SUITE_END() // TestTcpFactory
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace nfd
