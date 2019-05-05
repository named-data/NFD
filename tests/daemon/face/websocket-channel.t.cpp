/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "websocket-channel-fixture.hpp"
#include "face/websocket-transport.hpp"

#include "test-ip.hpp"

#include <boost/mpl/vector.hpp>

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestWebSocketChannel, WebSocketChannelFixture)

using AddressFamilies = boost::mpl::vector<
  std::integral_constant<AddressFamily, AddressFamily::V4>,
  std::integral_constant<AddressFamily, AddressFamily::V6>>;

BOOST_AUTO_TEST_CASE_TEMPLATE(Uri, F, AddressFamilies)
{
  using Address = typename IpAddressFromFamily<F::value>::type;
  websocket::Endpoint ep(Address::loopback(), 20070);
  auto channel = this->makeChannel(ep.address(), ep.port());
  BOOST_CHECK_EQUAL(channel->getUri(), FaceUri(ep, "ws"));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Listen, F, AddressFamilies)
{
  using Address = typename IpAddressFromFamily<F::value>::type;
  auto channel = this->makeChannel(Address());
  BOOST_CHECK_EQUAL(channel->isListening(), false);

  channel->listen(nullptr);
  BOOST_CHECK_EQUAL(channel->isListening(), true);

  // listen() is idempotent
  BOOST_CHECK_NO_THROW(channel->listen(nullptr));
  BOOST_CHECK_EQUAL(channel->isListening(), true);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(MultipleAccepts, F, AddressFamilies)
{
  auto address = getTestIp(F::value, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->listen(address);

  BOOST_CHECK_EQUAL(listenerChannel->isListening(), true);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);

  websocket::Client client1;
  this->clientConnect(client1);

  BOOST_CHECK_EQUAL(limitedIo.run(2, // listenerOnFaceCreated, clientHandleOpen
                                  1_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 1);

  websocket::Client client2;
  websocket::Client client3;
  this->clientConnect(client2);
  this->clientConnect(client3);

  BOOST_CHECK_EQUAL(limitedIo.run(4, // 2 listenerOnFaceCreated, 2 clientHandleOpen
                                  2_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 3);

  // check face persistency
  for (const auto& face : listenerFaces) {
    BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Send, F, AddressFamilies)
{
  auto address = getTestIp(F::value, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address);
  auto transport = listenerFaces.at(0)->getTransport();

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  transport->send(pkt1);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // clientHandleMessage
                                  1_s), LimitedIo::EXCEED_OPS);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world!");
  transport->send(pkt2);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // clientHandleMessage
                                  1_s), LimitedIo::EXCEED_OPS);

  BOOST_REQUIRE_EQUAL(clientReceivedMessages.size(), 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(
    reinterpret_cast<const uint8_t*>(clientReceivedMessages[0].data()),
    reinterpret_cast<const uint8_t*>(clientReceivedMessages[0].data()) + clientReceivedMessages[0].size(),
    pkt1.begin(), pkt1.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(
    reinterpret_cast<const uint8_t*>(clientReceivedMessages[1].data()),
    reinterpret_cast<const uint8_t*>(clientReceivedMessages[1].data()) + clientReceivedMessages[1].size(),
    pkt2.begin(), pkt2.end());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Receive, F, AddressFamilies)
{
  auto address = getTestIp(F::value, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address);

  // use network-layer packets here, otherwise GenericLinkService
  // won't recognize the packet type and will discard it
  auto interest1 = makeInterest("ndn:/TpnzGvW9R");
  auto interest2 = makeInterest("ndn:/QWiIMfj5sL");

  this->clientSendInterest(*interest1);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // faceAfterReceiveInterest
                                  1_s), LimitedIo::EXCEED_OPS);

  this->clientSendInterest(*interest2);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // faceAfterReceiveInterest
                                  1_s), LimitedIo::EXCEED_OPS);

  BOOST_REQUIRE_EQUAL(faceReceivedInterests.size(), 2);
  BOOST_CHECK_EQUAL(faceReceivedInterests[0].getName(), interest1->getName());
  BOOST_CHECK_EQUAL(faceReceivedInterests[1].getName(), interest2->getName());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(FaceClosure, F, AddressFamilies)
{
  auto address = getTestIp(F::value, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address);

  listenerFaces.at(0)->close();
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(RemoteClose, F, AddressFamilies)
{
  auto address = getTestIp(F::value, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address);

  client.close(clientHandle, websocketpp::close::status::going_away, "");
  BOOST_CHECK_EQUAL(limitedIo.run(1, // faceClosedSignal
                                  1_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SetPingInterval, F, AddressFamilies)
{
  auto address = getTestIp(F::value, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  const auto pingInterval = 1200_ms;
  this->initialize(address, pingInterval);

  BOOST_CHECK_EQUAL(limitedIo.run(2, // clientHandlePing
                                  pingInterval * 3), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_LE(measuredPingInterval, pingInterval * 1.1);
  BOOST_CHECK_GE(measuredPingInterval, pingInterval * 0.9);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SetPongTimeOut, F, AddressFamilies)
{
  auto address = getTestIp(F::value, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address, 600_ms, 300_ms);
  this->clientShouldPong = false;

  BOOST_CHECK_EQUAL(limitedIo.run(2, // clientHandlePing, faceClosedSignal
                                  2_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);

  auto transport = static_cast<WebSocketTransport*>(listenerFaces.at(0)->getTransport());
  BOOST_CHECK(transport->getState() == TransportState::FAILED ||
              transport->getState() == TransportState::CLOSED);
  BOOST_CHECK_GE(transport->getCounters().nOutPings, 1);
  BOOST_CHECK_LE(transport->getCounters().nOutPings, 2);
  BOOST_CHECK_EQUAL(transport->getCounters().nInPongs, 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestWebSocketChannel
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
