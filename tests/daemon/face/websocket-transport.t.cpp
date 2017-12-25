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

#include "websocket-transport-fixture.hpp"

#include <boost/mpl/vector.hpp>

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestWebSocketTransport, IpTransportFixture<WebSocketTransportFixture>)

using WebSocketTransportFixtures = boost::mpl::vector<
  GENERATE_IP_TRANSPORT_FIXTURE_INSTANTIATIONS(WebSocketTransportFixture)
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(StaticProperties, T, WebSocketTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  checkStaticPropertiesInitialized(*this->transport);

  BOOST_CHECK_EQUAL(this->transport->getLocalUri(), FaceUri(ip::tcp::endpoint(this->address, 20070), "ws"));
  BOOST_CHECK_EQUAL(this->transport->getRemoteUri(), FaceUri(this->remoteEp, "wsclient"));
  BOOST_CHECK_EQUAL(this->transport->getScope(),
                    this->addressScope == AddressScope::Loopback ? ndn::nfd::FACE_SCOPE_LOCAL
                                                                 : ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(this->transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(this->transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(this->transport->getMtu(), MTU_UNLIMITED);
  BOOST_CHECK_EQUAL(this->transport->getSendQueueCapacity(), QUEUE_UNSUPPORTED);
}

using StaticPropertiesV4MappedFixtures = boost::mpl::vector<
  IpTransportFixture<WebSocketTransportFixture, AddressFamily::V4, AddressScope::Loopback>,
  IpTransportFixture<WebSocketTransportFixture, AddressFamily::V4, AddressScope::Global>
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(StaticPropertiesV4Mapped, T, StaticPropertiesV4MappedFixtures, T)
{
  TRANSPORT_TEST_CHECK_PRECONDITIONS();
  auto mappedAddr = ip::address_v6::v4_mapped(this->address.to_v4());
  BOOST_REQUIRE(mappedAddr.is_v4_mapped());
  WebSocketTransportFixture::initialize(mappedAddr);

  checkStaticPropertiesInitialized(*this->transport);

  BOOST_CHECK_EQUAL(this->transport->getLocalUri(), FaceUri(ip::tcp::endpoint(mappedAddr, 20070), "ws"));
  BOOST_CHECK_EQUAL(this->transport->getRemoteUri(), FaceUri(this->remoteEp, "wsclient"));
  BOOST_CHECK_EQUAL(this->transport->getScope(),
                    this->addressScope == AddressScope::Loopback ? ndn::nfd::FACE_SCOPE_LOCAL
                                                                 : ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(this->transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(this->transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(this->transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(PersistencyChange)
{
  TRANSPORT_TEST_INIT();

  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND), true);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERSISTENT), false);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERMANENT), false);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(PingPong, T, WebSocketTransportFixtures, T)
{
  TRANSPORT_TEST_INIT(time::milliseconds(500), time::milliseconds(300));

  BOOST_CHECK_EQUAL(this->limitedIo.run(2, // clientHandlePing, serverHandlePong
                    time::milliseconds(1500)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nOutPings, 1);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPongs, 1);

  this->clientShouldPong = false;
  BOOST_CHECK_EQUAL(this->limitedIo.run(2, // clientHandlePing, serverHandlePongTimeout
                    time::seconds(2)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_MESSAGE(this->transport->getState() == TransportState::FAILED ||
                      this->transport->getState() == TransportState::CLOSED,
                      "expected FAILED or CLOSED state, actual state=" << this->transport->getState());
  BOOST_CHECK_EQUAL(this->transport->getCounters().nOutPings, 2);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPongs, 1);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(Send, T, WebSocketTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  auto block1 = ndn::encoding::makeStringBlock(300, "hello");
  this->transport->send(Transport::Packet{Block{block1}}); // make a copy of the block
  BOOST_CHECK_EQUAL(this->limitedIo.run(1, // clientHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nOutPackets, 1);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nOutBytes, block1.size());

  auto block2 = ndn::encoding::makeStringBlock(301, "world");
  this->transport->send(Transport::Packet{Block{block2}}); // make a copy of the block
  BOOST_CHECK_EQUAL(this->limitedIo.run(1, // clientHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nOutPackets, 2);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nOutBytes, block1.size() + block2.size());

  BOOST_REQUIRE_EQUAL(this->clientReceivedMessages.size(), 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(
    reinterpret_cast<const uint8_t*>(this->clientReceivedMessages[0].data()),
    reinterpret_cast<const uint8_t*>(this->clientReceivedMessages[0].data()) + this->clientReceivedMessages[0].size(),
    block1.begin(), block1.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(
    reinterpret_cast<const uint8_t*>(this->clientReceivedMessages[1].data()),
    reinterpret_cast<const uint8_t*>(this->clientReceivedMessages[1].data()) + this->clientReceivedMessages[1].size(),
    block2.begin(), block2.end());
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(ReceiveNormal, T, WebSocketTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  this->client.send(this->clientHdl, pkt1.wire(), pkt1.size(), websocketpp::frame::opcode::binary);
  BOOST_CHECK_EQUAL(this->limitedIo.run(1, // serverHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world!");
  this->client.send(this->clientHdl, pkt2.wire(), pkt2.size(), websocketpp::frame::opcode::binary);
  BOOST_CHECK_EQUAL(this->limitedIo.run(1, // serverHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(this->transport->getCounters().nInPackets, 2);
  BOOST_CHECK_EQUAL(this->transport->getCounters().nInBytes, pkt1.size() + pkt2.size());
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);

  BOOST_REQUIRE_EQUAL(this->serverReceivedPackets->size(), 2);
  BOOST_CHECK(this->serverReceivedPackets->at(0).packet == pkt1);
  BOOST_CHECK(this->serverReceivedPackets->at(1).packet == pkt2);
  BOOST_CHECK_EQUAL(this->serverReceivedPackets->at(0).remoteEndpoint,
                    this->serverReceivedPackets->at(1).remoteEndpoint);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(ReceiveMalformed, T, WebSocketTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  this->client.send(this->clientHdl, pkt1.wire(), pkt1.size() - 1, // truncated
                    websocketpp::frame::opcode::binary);
  BOOST_CHECK_EQUAL(this->limitedIo.run(1, // serverHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  // bad packet is dropped
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
  BOOST_CHECK_EQUAL(this->serverReceivedPackets->size(), 0);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world!");
  this->client.send(this->clientHdl, pkt2.wire(), pkt2.size(), websocketpp::frame::opcode::binary);
  BOOST_CHECK_EQUAL(this->limitedIo.run(1, // serverHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  // next valid packet is still received normally
  BOOST_CHECK_EQUAL(this->transport->getState(), TransportState::UP);
  BOOST_REQUIRE_EQUAL(this->serverReceivedPackets->size(), 1);
  BOOST_CHECK(this->serverReceivedPackets->at(0).packet == pkt2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(Close, T, WebSocketTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  int nStateChanges = 0;
  this->transport->afterStateChange.connect(
    [&nStateChanges] (TransportState oldState, TransportState newState) {
      switch (nStateChanges) {
      case 0:
        BOOST_CHECK_EQUAL(oldState, TransportState::UP);
        BOOST_CHECK_EQUAL(newState, TransportState::CLOSING);
        break;
      case 1:
        BOOST_CHECK_EQUAL(oldState, TransportState::CLOSING);
        BOOST_CHECK_EQUAL(newState, TransportState::CLOSED);
        break;
      default:
        BOOST_CHECK(false);
      }
      nStateChanges++;
    });

  this->transport->close();
  BOOST_CHECK_EQUAL(nStateChanges, 2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(RemoteClose, T, WebSocketTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  int nStateChanges = 0;
  this->transport->afterStateChange.connect(
    [&nStateChanges] (TransportState oldState, TransportState newState) {
      switch (nStateChanges) {
      case 0:
        BOOST_CHECK_EQUAL(oldState, TransportState::UP);
        BOOST_CHECK_EQUAL(newState, TransportState::CLOSING);
        break;
      case 1:
        BOOST_CHECK_EQUAL(oldState, TransportState::CLOSING);
        BOOST_CHECK_EQUAL(newState, TransportState::CLOSED);
        break;
      default:
        BOOST_CHECK(false);
      }
      nStateChanges++;
    });

  this->client.close(this->clientHdl, websocketpp::close::status::going_away, "");
  BOOST_CHECK_EQUAL(this->limitedIo.run(1, // serverHandleClose
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(nStateChanges, 2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(SendQueueLength, T, WebSocketTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  BOOST_CHECK_EQUAL(this->transport->getSendQueueLength(), QUEUE_UNSUPPORTED);
}

BOOST_AUTO_TEST_SUITE_END() // TestWebSocketTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
