/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#include "websocket-transport.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT("WebSocketTransport");

static bool
isLoopback(const boost::asio::ip::address& addr)
{
  if (addr.is_loopback()) {
    return true;
  }
  // Workaround for loopback IPv4-mapped IPv6 addresses
  // see https://svn.boost.org/trac/boost/ticket/9084
  else if (addr.is_v6()) {
    auto addr6 = addr.to_v6();
    if (addr6.is_v4_mapped()) {
      return addr6.to_v4().is_loopback();
    }
  }

  return false;
}

WebSocketTransport::WebSocketTransport(websocketpp::connection_hdl hdl,
                                       websocket::Server& server,
                                       time::milliseconds pingInterval)
  : m_handle(hdl)
  , m_server(server)
  , m_pingInterval(pingInterval)
{
  const auto& sock = m_server.get_con_from_hdl(hdl)->get_socket();
  this->setLocalUri(FaceUri(sock.local_endpoint(), "ws"));
  this->setRemoteUri(FaceUri(sock.remote_endpoint(), "wsclient"));

  if (isLoopback(sock.local_endpoint().address()) &&
      isLoopback(sock.remote_endpoint().address())) {
    this->setScope(ndn::nfd::FACE_SCOPE_LOCAL);
  }
  else {
    this->setScope(ndn::nfd::FACE_SCOPE_NON_LOCAL);
  }

  this->setPersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  this->setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  this->setMtu(MTU_UNLIMITED);

  this->schedulePing();

  NFD_LOG_FACE_INFO("Creating transport");
}

void
WebSocketTransport::doSend(Transport::Packet&& packet)
{
  NFD_LOG_FACE_TRACE(__func__);

  websocketpp::lib::error_code error;
  m_server.send(m_handle, packet.packet.wire(), packet.packet.size(),
                websocketpp::frame::opcode::binary, error);
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Successfully sent: " << packet.packet.size() << " bytes");
}

void
WebSocketTransport::receiveMessage(const std::string& msg)
{
  NFD_LOG_FACE_TRACE("Received: " << msg.size() << " bytes");

  bool isOk = false;
  Block element;
  std::tie(isOk, element) = Block::fromBuffer(reinterpret_cast<const uint8_t*>(msg.c_str()), msg.size());
  if (!isOk) {
    NFD_LOG_FACE_WARN("Failed to parse message payload");
    return;
  }

  this->receive(Transport::Packet(std::move(element)));
}

void
WebSocketTransport::schedulePing()
{
  m_pingEventId = scheduler::schedule(m_pingInterval, bind(&WebSocketTransport::sendPing, this));
}

void
WebSocketTransport::sendPing()
{
  NFD_LOG_FACE_TRACE(__func__);

  websocketpp::lib::error_code error;
  m_server.ping(m_handle, "NFD-WebSocket", error);
  if (error)
    return processErrorCode(error);

  ++this->nOutPings;

  this->schedulePing();
}

void
WebSocketTransport::handlePong()
{
  NFD_LOG_FACE_TRACE(__func__);

  ++this->nInPongs;
}

void
WebSocketTransport::handlePongTimeout()
{
  NFD_LOG_FACE_ERROR("Pong timeout");
  this->setState(TransportState::FAILED);
  doClose();
}

void
WebSocketTransport::processErrorCode(const websocketpp::lib::error_code& error)
{
  NFD_LOG_FACE_TRACE(__func__);

  if (getState() == TransportState::CLOSING ||
      getState() == TransportState::FAILED ||
      getState() == TransportState::CLOSED)
    // transport is shutting down, ignore any errors
    return;

  NFD_LOG_FACE_ERROR("Send or ping operation failed: " << error.message());
  this->setState(TransportState::FAILED);
  doClose();
}

void
WebSocketTransport::doClose()
{
  NFD_LOG_FACE_TRACE(__func__);

  m_pingEventId.cancel();

  // use the non-throwing variant and ignore errors, if any
  websocketpp::lib::error_code error;
  m_server.close(m_handle, websocketpp::close::status::normal, "closed by NFD", error);

  this->setState(TransportState::CLOSED);
}

} // namespace face
} // namespace nfd
