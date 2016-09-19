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

#include "face/websocket-transport.hpp"
#include "face/face.hpp"

#include "dummy-receive-link-service.hpp"
#include "test-ip.hpp"
#include "transport-test-common.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;
namespace ip = boost::asio::ip;

BOOST_AUTO_TEST_SUITE(Face)

using nfd::Face;

/** \brief a fixture that accepts a single WebSocket connection from a client
 */
class SingleWebSocketFixture : public BaseFixture
{
public:
  SingleWebSocketFixture()
    : transport(nullptr)
    , serverReceivedPackets(nullptr)
    , clientShouldPong(true)
  {
  }

  /** \brief initialize server and start listening
   */
  void
  serverListen(const ip::tcp::endpoint& ep,
               const time::milliseconds& pongTimeout = time::seconds(1))
  {
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.clear_error_channels(websocketpp::log::elevel::all);

    server.init_asio(&g_io);
    server.set_open_handler(bind(&SingleWebSocketFixture::serverHandleOpen, this, _1));
    server.set_close_handler(bind(&SingleWebSocketFixture::serverHandleClose, this));
    server.set_message_handler(bind(&SingleWebSocketFixture::serverHandleMessage, this, _2));
    server.set_pong_handler(bind(&SingleWebSocketFixture::serverHandlePong, this));
    server.set_pong_timeout_handler(bind(&SingleWebSocketFixture::serverHandlePongTimeout, this));
    server.set_pong_timeout(pongTimeout.count());

    server.set_reuse_addr(true);

    server.listen(ep);
    server.start_accept();
  }

  /** \brief initialize client and connect to server
   */
  void
  clientConnect(const std::string& uri)
  {
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.clear_error_channels(websocketpp::log::elevel::all);

    client.init_asio(&g_io);
    client.set_open_handler(bind(&SingleWebSocketFixture::clientHandleOpen, this, _1));
    client.set_message_handler(bind(&SingleWebSocketFixture::clientHandleMessage, this, _2));
    client.set_ping_handler(bind(&SingleWebSocketFixture::clientHandlePing, this));

    websocketpp::lib::error_code ec;
    auto con = client.get_connection(uri, ec);
    BOOST_REQUIRE_EQUAL(ec, websocketpp::lib::error_code());

    client.connect(con);
  }

  void
  makeFace(const time::milliseconds& pingInterval = time::seconds(10))
  {
    face = make_unique<Face>(
             make_unique<DummyReceiveLinkService>(),
             make_unique<WebSocketTransport>(serverHdl, ref(server), pingInterval));
    transport = static_cast<WebSocketTransport*>(face->getTransport());
    serverReceivedPackets = &static_cast<DummyReceiveLinkService*>(face->getLinkService())->receivedPackets;
  }

  /** \brief initialize both server and client, and have each other connected, create Transport
   */
  void
  endToEndInitialize(const ip::tcp::endpoint& ep,
                     const time::milliseconds& pingInterval = time::seconds(10),
                     const time::milliseconds& pongTimeout = time::seconds(1))
  {
    this->serverListen(ep, pongTimeout);
    std::string uri;
    if (ep.address().is_v6()) {
      uri = "ws://[" + ep.address().to_string() + "]:" + to_string(ep.port());
    }
    else {
      uri = "ws://" + ep.address().to_string() + ":" + to_string(ep.port());
    }
    this->clientConnect(uri);
    BOOST_REQUIRE_EQUAL(limitedIo.run(2, // serverHandleOpen, clientHandleOpen
                        time::seconds(1)), LimitedIo::EXCEED_OPS);
    this->makeFace(pingInterval);
  }

private:
  void
  serverHandleOpen(websocketpp::connection_hdl hdl)
  {
    websocketpp::lib::error_code ec;
    auto con = server.get_con_from_hdl(hdl, ec);
    BOOST_REQUIRE_EQUAL(ec, websocketpp::lib::error_code());
    BOOST_REQUIRE(con);
    remoteEp = con->get_socket().remote_endpoint();

    serverHdl = hdl;
    limitedIo.afterOp();
  }

  void
  serverHandleClose()
  {
    if (transport == nullptr) {
      return;
    }

    transport->close();
    limitedIo.afterOp();
  }

  void
  serverHandleMessage(websocket::Server::message_ptr msg)
  {
    if (transport == nullptr) {
      return;
    }

    transport->receiveMessage(msg->get_payload());
    limitedIo.afterOp();
  }

  void
  serverHandlePong()
  {
    if (transport == nullptr) {
      return;
    }

    transport->handlePong();
    limitedIo.afterOp();
  }

  void
  serverHandlePongTimeout()
  {
    if (transport == nullptr) {
      return;
    }

    transport->handlePongTimeout();
    limitedIo.afterOp();
  }

  void
  clientHandleOpen(websocketpp::connection_hdl hdl)
  {
    clientHdl = hdl;
    limitedIo.afterOp();
  }

  void
  clientHandleMessage(websocket::Client::message_ptr msg)
  {
    clientReceivedMessages.push_back(msg->get_payload());
    limitedIo.afterOp();
  }

  bool
  clientHandlePing()
  {
    limitedIo.afterOp();
    return clientShouldPong;
  }

public:
  LimitedIo limitedIo;

  websocket::Server server;
  websocketpp::connection_hdl serverHdl;
  ip::tcp::endpoint remoteEp;
  unique_ptr<Face> face;
  WebSocketTransport* transport;
  std::vector<Transport::Packet>* serverReceivedPackets;

  websocket::Client client;
  websocketpp::connection_hdl clientHdl;
  bool clientShouldPong;
  std::vector<std::string> clientReceivedMessages;
};

BOOST_FIXTURE_TEST_SUITE(TestWebSocketTransport, SingleWebSocketFixture)

BOOST_AUTO_TEST_CASE(StaticPropertiesLocalIpv4)
{
  auto address = getTestIp<ip::address_v4>(LoopbackAddress::Yes);
  SKIP_IF_IP_UNAVAILABLE(address);
  ip::tcp::endpoint ep(address, 20070);
  this->endToEndInitialize(ep);

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(), FaceUri(ep, "ws"));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(), FaceUri(remoteEp, "wsclient"));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(StaticPropertiesNonLocalIpv4)
{
  auto address = getTestIp<ip::address_v4>(LoopbackAddress::No);
  SKIP_IF_IP_UNAVAILABLE(address);
  ip::tcp::endpoint ep(address, 20070);
  this->endToEndInitialize(ep);

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(), FaceUri(ep, "ws"));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(), FaceUri(remoteEp, "wsclient"));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(StaticPropertiesLocalIpv4MappedIpv6)
{
  auto address4 = getTestIp<ip::address_v4>(LoopbackAddress::Yes);
  SKIP_IF_IP_UNAVAILABLE(address4);
  auto address6 = ip::address_v6::v4_mapped(address4);
  BOOST_REQUIRE(address6.is_v4_mapped());
  ip::tcp::endpoint ep(address6, 20070);
  this->endToEndInitialize(ep);

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(), FaceUri(ep, "ws"));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(), FaceUri(remoteEp, "wsclient"));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(PingPong)
{
  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);
  this->endToEndInitialize(ip::tcp::endpoint(address, 20070),
                           time::milliseconds(500), time::milliseconds(300));

  BOOST_CHECK_EQUAL(limitedIo.run(2, // clientHandlePing, serverHandlePong
                    time::milliseconds(1500)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);
  BOOST_CHECK_EQUAL(transport->getCounters().nOutPings, 1);
  BOOST_CHECK_EQUAL(transport->getCounters().nInPongs, 1);

  this->clientShouldPong = false;
  BOOST_CHECK_EQUAL(limitedIo.run(2, // clientHandlePing, serverHandlePongTimeout
                    time::seconds(2)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_MESSAGE(transport->getState() == TransportState::FAILED ||
                      transport->getState() == TransportState::CLOSED,
                      "expect FAILED or CLOSED state, actual state=" << transport->getState());
  BOOST_CHECK_EQUAL(transport->getCounters().nOutPings, 2);
  BOOST_CHECK_EQUAL(transport->getCounters().nInPongs, 1);
}

BOOST_AUTO_TEST_CASE(Send)
{
  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);
  this->endToEndInitialize(ip::tcp::endpoint(address, 20070));

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  transport->send(Transport::Packet(Block(pkt1)));
  BOOST_CHECK_EQUAL(limitedIo.run(1, // clientHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world!");
  transport->send(Transport::Packet(Block(pkt2)));
  BOOST_CHECK_EQUAL(limitedIo.run(1, // clientHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

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

BOOST_AUTO_TEST_CASE(ReceiveNormal)
{
  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);
  this->endToEndInitialize(ip::tcp::endpoint(address, 20070));

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  client.send(clientHdl, pkt1.wire(), pkt1.size(), websocketpp::frame::opcode::binary);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // serverHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world!");
  client.send(clientHdl, pkt2.wire(), pkt2.size(), websocketpp::frame::opcode::binary);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // serverHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_REQUIRE_EQUAL(serverReceivedPackets->size(), 2);
  BOOST_CHECK(serverReceivedPackets->at(0).packet == pkt1);
  BOOST_CHECK(serverReceivedPackets->at(1).packet == pkt2);
  BOOST_CHECK_EQUAL(serverReceivedPackets->at(0).remoteEndpoint, serverReceivedPackets->at(1).remoteEndpoint);
}

BOOST_AUTO_TEST_CASE(ReceiveMalformed)
{
  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);
  this->endToEndInitialize(ip::tcp::endpoint(address, 20070));

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  client.send(clientHdl, pkt1.wire(), pkt1.size() - 1, // truncated
              websocketpp::frame::opcode::binary);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // serverHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  // bad packet is dropped
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);
  BOOST_CHECK_EQUAL(serverReceivedPackets->size(), 0);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "world!");
  client.send(clientHdl, pkt2.wire(), pkt2.size(), websocketpp::frame::opcode::binary);
  BOOST_CHECK_EQUAL(limitedIo.run(1, // serverHandleMessage
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  // next valid packet is still received normally
  BOOST_REQUIRE_EQUAL(serverReceivedPackets->size(), 1);
  BOOST_CHECK(serverReceivedPackets->at(0).packet == pkt2);
}

BOOST_AUTO_TEST_CASE(Close)
{
  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);
  this->endToEndInitialize(ip::tcp::endpoint(address, 20070));

  int nStateChanges = 0;
  transport->afterStateChange.connect(
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

  transport->close();
  BOOST_CHECK_EQUAL(nStateChanges, 2);
}

BOOST_AUTO_TEST_CASE(RemoteClose)
{
  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);
  this->endToEndInitialize(ip::tcp::endpoint(address, 20070));

  int nStateChanges = 0;
  transport->afterStateChange.connect(
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

  client.close(clientHdl, websocketpp::close::status::going_away, "");
  BOOST_CHECK_EQUAL(limitedIo.run(1, // serverHandleClose
                    time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(nStateChanges, 2);
}

BOOST_AUTO_TEST_SUITE_END() // TestWebSocketTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
