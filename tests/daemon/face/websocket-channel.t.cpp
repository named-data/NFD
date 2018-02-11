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

#include "face/websocket-channel.hpp"
#include "face/websocket-transport.hpp"

#include "channel-fixture.hpp"
#include "test-ip.hpp"

namespace nfd {
namespace face {
namespace tests {

namespace ip = boost::asio::ip;

class WebSocketChannelFixture : public ChannelFixture<WebSocketChannel, websocket::Endpoint>
{
protected:
  unique_ptr<WebSocketChannel>
  makeChannel(const ip::address& addr, uint16_t port = 0) final
  {
    if (port == 0)
      port = getNextPort();

    return make_unique<WebSocketChannel>(websocket::Endpoint(addr, port));
  }

  void
  listen(const ip::address& addr,
         const time::milliseconds& pingInterval = 10_s,
         const time::milliseconds& pongTimeout = 1_s)
  {
    listenerEp = websocket::Endpoint(addr, 20030);
    listenerChannel = makeChannel(addr, 20030);
    listenerChannel->setPingInterval(pingInterval);
    listenerChannel->setPongTimeout(pongTimeout);
    listenerChannel->listen(bind(&WebSocketChannelFixture::listenerOnFaceCreated, this, _1));
  }

  void
  clientConnect(websocket::Client& client)
  {
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.clear_error_channels(websocketpp::log::elevel::all);

    client.init_asio(&g_io);
    client.set_open_handler(bind(&WebSocketChannelFixture::clientHandleOpen, this, _1));
    client.set_message_handler(bind(&WebSocketChannelFixture::clientHandleMessage, this, _1, _2));
    client.set_ping_handler(bind(&WebSocketChannelFixture::clientHandlePing, this, _1, _2));

    std::string uri = "ws://" + listenerEp.address().to_string() + ":" + to_string(listenerEp.port());
    websocketpp::lib::error_code ec;
    auto con = client.get_connection(uri, ec);
    BOOST_REQUIRE_EQUAL(ec, websocketpp::lib::error_code());

    client.connect(con);
  }

  void
  initialize(const ip::address& addr,
             const time::milliseconds& pingInterval = 10_s,
             const time::milliseconds& pongTimeout = 1_s)
  {
    listen(addr, pingInterval, pongTimeout);
    clientConnect(client);
    BOOST_REQUIRE_EQUAL(limitedIo.run(2, // listenerOnFaceCreated, clientHandleOpen
                                      1_s), LimitedIo::EXCEED_OPS);
    BOOST_REQUIRE_EQUAL(listenerChannel->size(), 1);
  }

  void
  clientSendInterest(const Interest& interest)
  {
    const Block& payload = interest.wireEncode();
    client.send(clientHandle, payload.wire(), payload.size(), websocketpp::frame::opcode::binary);
  }

private:
  void
  listenerOnFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_REQUIRE(newFace != nullptr);
    newFace->afterReceiveInterest.connect(bind(&WebSocketChannelFixture::faceAfterReceiveInterest, this, _1));
    connectFaceClosedSignal(*newFace, [this] { limitedIo.afterOp(); });
    listenerFaces.push_back(newFace);
    limitedIo.afterOp();
  }

  void
  faceAfterReceiveInterest(const Interest& interest)
  {
    faceReceivedInterests.push_back(interest);
    limitedIo.afterOp();
  }

  void
  clientHandleOpen(websocketpp::connection_hdl hdl)
  {
    clientHandle = hdl;
    limitedIo.afterOp();
  }

  void
  clientHandleMessage(websocketpp::connection_hdl, websocket::Client::message_ptr msg)
  {
    clientReceivedMessages.push_back(msg->get_payload());
    limitedIo.afterOp();
  }

  bool
  clientHandlePing(websocketpp::connection_hdl, std::string)
  {
    auto now = time::steady_clock::now();
    if (m_prevPingRecvTime != time::steady_clock::TimePoint()) {
      measuredPingInterval = now - m_prevPingRecvTime;
    }
    m_prevPingRecvTime = now;

    limitedIo.afterOp();
    return clientShouldPong;
  }

protected:
  std::vector<Interest> faceReceivedInterests;

  websocket::Client client;
  websocketpp::connection_hdl clientHandle;
  std::vector<std::string> clientReceivedMessages;

  time::steady_clock::Duration measuredPingInterval;
  // set clientShouldPong to false to disable the pong response,
  // which will eventually cause a timeout in listenerChannel
  bool clientShouldPong = true;

private:
  time::steady_clock::TimePoint m_prevPingRecvTime;
};

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestWebSocketChannel, WebSocketChannelFixture)

BOOST_AUTO_TEST_CASE(Uri)
{
  websocket::Endpoint ep(ip::address_v4::loopback(), 20070);
  auto channel = makeChannel(ep.address(), ep.port());
  BOOST_CHECK_EQUAL(channel->getUri(), FaceUri(ep, "ws"));
}

BOOST_AUTO_TEST_CASE(Listen)
{
  auto channel = makeChannel(ip::address_v4());
  BOOST_CHECK_EQUAL(channel->isListening(), false);

  channel->listen(nullptr);
  BOOST_CHECK_EQUAL(channel->isListening(), true);

  // listen() is idempotent
  BOOST_CHECK_NO_THROW(channel->listen(nullptr));
  BOOST_CHECK_EQUAL(channel->isListening(), true);
}

BOOST_AUTO_TEST_CASE(MultipleAccepts)
{
  auto address = getTestIp(AddressFamily::V4, AddressScope::Loopback);
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

BOOST_AUTO_TEST_CASE(Send)
{
  auto address = getTestIp(AddressFamily::V4, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address);
  auto transport = listenerFaces.front()->getTransport();

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  transport->send(Transport::Packet(Block(pkt1)));
  BOOST_CHECK_EQUAL(limitedIo.run(1, // clientHandleMessage
                                  1_s), LimitedIo::EXCEED_OPS);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world!");
  transport->send(Transport::Packet(Block(pkt2)));
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

BOOST_AUTO_TEST_CASE(Receive)
{
  auto address = getTestIp(AddressFamily::V4, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address);

  // use network-layer packets here, otherwise GenericLinkService
  // won't recognize the packet type and will discard it
  auto interest1 = makeInterest("ndn:/TpnzGvW9R");
  auto interest2 = makeInterest("ndn:/QWiIMfj5sL");

  clientSendInterest(*interest1);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // faceAfterReceiveInterest
                                  1_s), LimitedIo::EXCEED_OPS);

  clientSendInterest(*interest2);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // faceAfterReceiveInterest
                                  1_s), LimitedIo::EXCEED_OPS);

  BOOST_REQUIRE_EQUAL(faceReceivedInterests.size(), 2);
  BOOST_CHECK_EQUAL(faceReceivedInterests[0].getName(), interest1->getName());
  BOOST_CHECK_EQUAL(faceReceivedInterests[1].getName(), interest2->getName());
}

BOOST_AUTO_TEST_CASE(FaceClosure)
{
  auto address = getTestIp(AddressFamily::V4, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address);

  listenerFaces.front()->close();
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);
}

BOOST_AUTO_TEST_CASE(RemoteClose)
{
  auto address = getTestIp(AddressFamily::V4, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address);

  client.close(clientHandle, websocketpp::close::status::going_away, "");
  BOOST_CHECK_EQUAL(limitedIo.run(1, // faceClosedSignal
                                  1_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);
}

BOOST_AUTO_TEST_CASE(SetPingInterval)
{
  auto address = getTestIp(AddressFamily::V4, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  const auto pingInterval = 1200_ms;
  this->initialize(address, pingInterval);

  BOOST_CHECK_EQUAL(limitedIo.run(2, // clientHandlePing
                                  pingInterval * 3), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_LE(measuredPingInterval, pingInterval * 1.1);
  BOOST_CHECK_GE(measuredPingInterval, pingInterval * 0.9);
}

BOOST_AUTO_TEST_CASE(SetPongTimeOut)
{
  auto address = getTestIp(AddressFamily::V4, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->initialize(address, 600_ms, 300_ms);
  clientShouldPong = false;

  BOOST_CHECK_EQUAL(limitedIo.run(2, // clientHandlePing, faceClosedSignal
                                  2_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);

  auto transport = static_cast<WebSocketTransport*>(listenerFaces.front()->getTransport());
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
