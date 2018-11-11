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

#ifndef NFD_TESTS_DAEMON_FACE_WEBSOCKET_CHANNEL_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_WEBSOCKET_CHANNEL_FIXTURE_HPP

#include "face/websocket-channel.hpp"

#include "channel-fixture.hpp"

namespace nfd {
namespace face {
namespace tests {

class WebSocketChannelFixture : public ChannelFixture<WebSocketChannel, websocket::Endpoint>
{
protected:
  unique_ptr<WebSocketChannel>
  makeChannel(const boost::asio::ip::address& addr, uint16_t port = 0) final
  {
    if (port == 0)
      port = getNextPort();

    return make_unique<WebSocketChannel>(websocket::Endpoint(addr, port));
  }

  void
  listen(const boost::asio::ip::address& addr,
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

    websocketpp::lib::error_code ec;
    auto con = client.get_connection(FaceUri(listenerEp, "ws").toString(), ec);
    BOOST_REQUIRE_EQUAL(ec, websocketpp::lib::error_code());

    client.connect(con);
  }

  void
  initialize(const boost::asio::ip::address& addr,
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

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_WEBSOCKET_CHANNEL_FIXTURE_HPP
