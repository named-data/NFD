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

#include "transport-test-common.hpp"

#include "multicast-udp-transport-fixture.hpp"

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestMulticastUdpTransport, MulticastUdpTransportFixture)

BOOST_AUTO_TEST_CASE(StaticPropertiesNonLocalIpv4)
{
  SKIP_IF_IP_UNAVAILABLE(defaultAddr);
  initialize(defaultAddr);

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(),
                    FaceUri("udp4://" + defaultAddr.to_string() + ":" + to_string(localEp.port())));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(),
                    FaceUri("udp4://" + multicastEp.address().to_string() + ":" + to_string(multicastEp.port())));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  BOOST_CHECK_EQUAL(transport->getMtu(), 65535 - 60 - 8);
}

BOOST_AUTO_TEST_CASE(ReceiveMultipleRemoteEndpoints)
{
  SKIP_IF_IP_UNAVAILABLE(defaultAddr);
  initialize(defaultAddr);

  // remoteSockRx2 unnecessary for this test case - only remoteSockTx2 is needed
  udp::socket remoteSockTx2(g_io);
  remoteSockTx2.open(udp::v4());
  remoteSockTx2.set_option(udp::socket::reuse_address(true));
  remoteSockTx2.set_option(ip::multicast::enable_loopback(true));
  remoteSockTx2.bind(udp::endpoint(ip::address_v4::any(), 7071));

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  ndn::Buffer buf1(pkt1.begin(), pkt1.end());
  remoteWrite(buf1);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world");
  ndn::Buffer buf2(pkt2.begin(), pkt2.end());
  remoteWrite(buf2);

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 2);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, buf1.size() + buf2.size());
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);

  BOOST_REQUIRE_EQUAL(receivedPackets->size(), 2);
  BOOST_CHECK_EQUAL(receivedPackets->at(0).remoteEndpoint,
                    receivedPackets->at(1).remoteEndpoint);

  udp::endpoint destEp(multicastEp.address(), localEp.port());
  remoteSockTx2.async_send_to(boost::asio::buffer(buf1), destEp,
    [] (const boost::system::error_code& error, size_t) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
    });
  remoteSockTx2.async_send_to(boost::asio::buffer(buf2), destEp,
    [] (const boost::system::error_code& error, size_t) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
    });
  limitedIo.defer(time::seconds(1));

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 4);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, 2 * buf1.size() + 2 * buf2.size());
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);

  BOOST_REQUIRE_EQUAL(receivedPackets->size(), 4);
  BOOST_CHECK_EQUAL(receivedPackets->at(2).remoteEndpoint,
                    receivedPackets->at(3).remoteEndpoint);
  BOOST_CHECK_NE(receivedPackets->at(0).remoteEndpoint,
                 receivedPackets->at(2).remoteEndpoint);
}

BOOST_AUTO_TEST_SUITE_END() // TestMulticastUdpTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
