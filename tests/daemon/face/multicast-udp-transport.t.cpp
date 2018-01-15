/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include <boost/mpl/vector.hpp>

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)

using MulticastUdpTransportFixtureWithAddress =
  IpTransportFixture<MulticastUdpTransportFixture, AddressFamily::Any,
                     AddressScope::Global, MulticastInterface::Yes>;

BOOST_FIXTURE_TEST_SUITE(TestMulticastUdpTransport, MulticastUdpTransportFixtureWithAddress)

using MulticastUdpTransportFixtures = boost::mpl::vector<
  IpTransportFixture<MulticastUdpTransportFixture, AddressFamily::V4, AddressScope::Global, MulticastInterface::Yes>,
  IpTransportFixture<MulticastUdpTransportFixture, AddressFamily::V6, AddressScope::LinkLocal, MulticastInterface::Yes>,
  IpTransportFixture<MulticastUdpTransportFixture, AddressFamily::V6, AddressScope::Global, MulticastInterface::Yes>
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(StaticProperties, T, MulticastUdpTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  checkStaticPropertiesInitialized(*this->transport);

  BOOST_CHECK_EQUAL(this->transport->getLocalUri(), FaceUri(udp::endpoint(this->address, this->txPort)));
  BOOST_CHECK_EQUAL(this->transport->getRemoteUri(), FaceUri(this->mcastEp));
  BOOST_CHECK_EQUAL(this->transport->getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(this->transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(this->transport->getLinkType(), ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  BOOST_CHECK_EQUAL(this->transport->getMtu(),
                    this->addressFamily == AddressFamily::V4 ? (65535 - 60 - 8) : (65535 - 8));
  BOOST_CHECK_GT(this->transport->getSendQueueCapacity(), 0);
}

BOOST_AUTO_TEST_CASE(PersistencyChange)
{
  TRANSPORT_TEST_INIT();

  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND), false);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERSISTENT), false);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERMANENT), true);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(ReceiveMultipleRemoteEndpoints, T, MulticastUdpTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  // we need a second remote tx socket for this test case
  udp::socket remoteSockTx2(this->g_io);
  MulticastUdpTransport::openTxSocket(remoteSockTx2, udp::endpoint(this->address, 0), nullptr, true);

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  ndn::Buffer buf1(pkt1.begin(), pkt1.end());
  this->remoteWrite(buf1);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world");
  ndn::Buffer buf2(pkt2.begin(), pkt2.end());
  this->remoteWrite(buf2);

  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPackets, 2);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInBytes, buf1.size() + buf2.size());
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);

  BOOST_REQUIRE_EQUAL(this->receivedPackets->size(), 2);
  BOOST_CHECK_EQUAL(this->receivedPackets->at(0).remoteEndpoint,
                    this->receivedPackets->at(1).remoteEndpoint);

  this->sendToGroup(remoteSockTx2, buf1);
  this->sendToGroup(remoteSockTx2, buf2);
  this->limitedIo.defer(time::seconds(1));

  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPackets, 4);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInBytes, 2 * buf1.size() + 2 * buf2.size());
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);

  BOOST_REQUIRE_EQUAL(this->receivedPackets->size(), 4);
  BOOST_CHECK_EQUAL(this->receivedPackets->at(2).remoteEndpoint,
                    this->receivedPackets->at(3).remoteEndpoint);
  BOOST_CHECK_NE(this->receivedPackets->at(0).remoteEndpoint,
                 this->receivedPackets->at(2).remoteEndpoint);
}

BOOST_AUTO_TEST_SUITE_END() // TestMulticastUdpTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
