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

#include "unicast-udp-transport-fixture.hpp"
#include "multicast-udp-transport-fixture.hpp"

#include "transport-test-common.hpp"

#include <boost/mpl/vector.hpp>

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_AUTO_TEST_SUITE(TestDatagramTransport)

using DatagramTransportFixtures = boost::mpl::vector<
  GENERATE_IP_TRANSPORT_FIXTURE_INSTANTIATIONS(UnicastUdpTransportFixture),
  IpTransportFixture<MulticastUdpTransportFixture, AddressFamily::V4, AddressScope::Global, MulticastInterface::Yes>,
  IpTransportFixture<MulticastUdpTransportFixture, AddressFamily::V6, AddressScope::LinkLocal, MulticastInterface::Yes>,
  IpTransportFixture<MulticastUdpTransportFixture, AddressFamily::V6, AddressScope::Global, MulticastInterface::Yes>
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(Send, T, DatagramTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  auto block1 = ndn::encoding::makeStringBlock(300, "hello");
  this->transport->send(Transport::Packet{Block{block1}}); // make a copy of the block
  BOOST_CHECK_EQUAL(this->transport->getCounters().nOutPackets, 1);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nOutBytes, block1.size());

  std::vector<uint8_t> readBuf(block1.size());
  this->remoteRead(readBuf);

  BOOST_CHECK_EQUAL_COLLECTIONS(readBuf.begin(), readBuf.end(), block1.begin(), block1.end());
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(ReceiveNormal, T, DatagramTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  Block pkt = ndn::encoding::makeStringBlock(300, "hello");
  ndn::Buffer buf(pkt.begin(), pkt.end());
  this->remoteWrite(buf);

  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPackets, 1);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInBytes, pkt.size());
  BOOST_CHECK_EQUAL(this->receivedPackets->size(), 1);
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(ReceiveIncomplete, T, DatagramTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  this->remoteWrite({0x05, 0x03, 0x00, 0x01});

  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPackets, 0);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInBytes, 0);
  BOOST_CHECK_EQUAL(this->receivedPackets->size(), 0);
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(ReceiveTrailingGarbage, T, DatagramTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  Block pkt2 = ndn::encoding::makeStringBlock(301, "world");
  ndn::Buffer buf(pkt1.size() + pkt2.size());
  std::copy(pkt1.begin(), pkt1.end(), buf.begin());
  std::copy(pkt2.begin(), pkt2.end(), buf.begin() + pkt1.size());

  this->remoteWrite(buf);

  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPackets, 0);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInBytes, 0);
  BOOST_CHECK_EQUAL(this->receivedPackets->size(), 0);
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(ReceiveTooLarge, T, DatagramTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  std::vector<uint8_t> bytes(ndn::MAX_NDN_PACKET_SIZE, 0);
  Block pkt1 = ndn::encoding::makeBinaryBlock(300, bytes.data(), bytes.size() - 6);
  ndn::Buffer buf1(pkt1.begin(), pkt1.end());
  BOOST_REQUIRE_EQUAL(buf1.size(), ndn::MAX_NDN_PACKET_SIZE);

  Block pkt2 = ndn::encoding::makeBinaryBlock(301, bytes.data(), bytes.size());
  ndn::Buffer buf2(pkt2.begin(), pkt2.end());
  BOOST_REQUIRE_GT(buf2.size(), ndn::MAX_NDN_PACKET_SIZE);

  this->remoteWrite(buf1); // this should succeed

  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPackets, 1);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInBytes, buf1.size());
  BOOST_CHECK_EQUAL(this->receivedPackets->size(), 1);
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);

  this->remoteWrite(buf2, false); // this should fail

  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPackets, 1);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInBytes, buf1.size());
  BOOST_CHECK_EQUAL(this->receivedPackets->size(), 1);
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(Close, T, DatagramTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  this->transport->afterStateChange.connectSingleShot([] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::UP);
    BOOST_CHECK_EQUAL(newState, TransportState::CLOSING);
  });

  this->transport->close();

  this->transport->afterStateChange.connectSingleShot([this] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::CLOSING);
    BOOST_CHECK_EQUAL(newState, TransportState::CLOSED);
    this->limitedIo.afterOp();
  });

  BOOST_REQUIRE_EQUAL(this->limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(SendQueueLength, T, DatagramTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  BOOST_CHECK_EQUAL(this->transport->getSendQueueLength(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestDatagramTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
