/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_FACE_WEBSOCKET_TRANSPORT_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_WEBSOCKET_TRANSPORT_FIXTURE_HPP

#include "face/websocket-transport.hpp"
#include "face/face.hpp"

#include "dummy-receive-link-service.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;
namespace ip = boost::asio::ip;

/** \brief a fixture that accepts a single WebSocket connection from a client
 */
class WebSocketTransportFixture : public BaseFixture
{
protected:
  WebSocketTransportFixture()
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
    server.set_open_handler(bind(&WebSocketTransportFixture::serverHandleOpen, this, _1));
    server.set_close_handler(bind(&WebSocketTransportFixture::serverHandleClose, this));
    server.set_message_handler(bind(&WebSocketTransportFixture::serverHandleMessage, this, _2));
    server.set_pong_handler(bind(&WebSocketTransportFixture::serverHandlePong, this));
    server.set_pong_timeout_handler(bind(&WebSocketTransportFixture::serverHandlePongTimeout, this));
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
    client.set_open_handler(bind(&WebSocketTransportFixture::clientHandleOpen, this, _1));
    client.set_message_handler(bind(&WebSocketTransportFixture::clientHandleMessage, this, _2));
    client.set_ping_handler(bind(&WebSocketTransportFixture::clientHandlePing, this));

    websocketpp::lib::error_code ec;
    auto con = client.get_connection(uri, ec);
    BOOST_REQUIRE_EQUAL(ec, websocketpp::lib::error_code());

    client.connect(con);
  }

  /** \brief initialize both server and client, and have each other connected, create Transport
   */
  void
  initialize(ip::address address,
             time::milliseconds pingInterval = time::seconds(10),
             time::milliseconds pongTimeout = time::seconds(1))
  {
    ip::tcp::endpoint ep(address, 20070);
    serverListen(ep, pongTimeout);
    clientConnect(FaceUri(ep, "ws").toString());

    BOOST_REQUIRE_EQUAL(limitedIo.run(2, // serverHandleOpen, clientHandleOpen
                        time::seconds(1)), LimitedIo::EXCEED_OPS);

    face = make_unique<Face>(
             make_unique<DummyReceiveLinkService>(),
             make_unique<WebSocketTransport>(serverHdl, ref(server), pingInterval));
    transport = static_cast<WebSocketTransport*>(face->getTransport());
    serverReceivedPackets = &static_cast<DummyReceiveLinkService*>(face->getLinkService())->receivedPackets;

    BOOST_REQUIRE_EQUAL(transport->getState(), TransportState::UP);
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

protected:
  LimitedIo limitedIo;

  websocket::Server server;
  websocketpp::connection_hdl serverHdl;
  ip::tcp::endpoint remoteEp;
  WebSocketTransport* transport;
  std::vector<Transport::Packet>* serverReceivedPackets;

  websocket::Client client;
  websocketpp::connection_hdl clientHdl;
  bool clientShouldPong;
  std::vector<std::string> clientReceivedMessages;

private:
  unique_ptr<Face> face;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_WEBSOCKET_TRANSPORT_FIXTURE_HPP
