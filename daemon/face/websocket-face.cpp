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

#include "websocket-face.hpp"

namespace nfd {

NFD_LOG_INIT("WebSocketFace");

WebSocketFace::WebSocketFace(const FaceUri& remoteUri, const FaceUri& localUri,
                             websocketpp::connection_hdl hdl,
                             websocket::Server& server)
  : Face(remoteUri, localUri)
  , m_handle(hdl)
  , m_server(server)
  , m_closed(false)
{
  NFD_LOG_FACE_INFO("Creating face");
  this->setPersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
}

void
WebSocketFace::sendInterest(const Interest& interest)
{
  if (m_closed)
    return;

  NFD_LOG_FACE_TRACE(__func__);

  this->emitSignal(onSendInterest, interest);

  const Block& payload = interest.wireEncode();
  this->getMutableCounters().getNOutBytes() += payload.size();

  websocketpp::lib::error_code ec;
  m_server.send(m_handle, payload.wire(), payload.size(),
                websocketpp::frame::opcode::binary, ec);
  if (ec)
    NFD_LOG_FACE_WARN("Failed to send Interest: " << ec.message());
}

void
WebSocketFace::sendData(const Data& data)
{
  if (m_closed)
    return;

  NFD_LOG_FACE_TRACE(__func__);

  this->emitSignal(onSendData, data);

  const Block& payload = data.wireEncode();
  this->getMutableCounters().getNOutBytes() += payload.size();

  websocketpp::lib::error_code ec;
  m_server.send(m_handle, payload.wire(), payload.size(),
                websocketpp::frame::opcode::binary, ec);
  if (ec)
    NFD_LOG_FACE_WARN("Failed to send Data: " << ec.message());
}

void
WebSocketFace::close()
{
  if (m_closed)
    return;

  NFD_LOG_FACE_INFO("Closing face");

  m_closed = true;
  scheduler::cancel(m_pingEventId);
  websocketpp::lib::error_code ec;
  m_server.close(m_handle, websocketpp::close::status::normal, "closed by nfd", ec);
  // ignore error on close
  fail("Face closed");
}

void
WebSocketFace::handleReceive(const std::string& msg)
{
  // Copy message into Face internal buffer
  if (msg.size() > ndn::MAX_NDN_PACKET_SIZE)
    {
      NFD_LOG_FACE_WARN("Received WebSocket message is too big (" << msg.size() << " bytes)");
      return;
    }

  NFD_LOG_FACE_TRACE("Received: " << msg.size() << " bytes");
  this->getMutableCounters().getNInBytes() += msg.size();

  // Try to parse message data
  bool isOk = false;
  Block element;
  std::tie(isOk, element) = Block::fromBuffer(reinterpret_cast<const uint8_t*>(msg.c_str()),
                                              msg.size());
  if (!isOk)
    {
      NFD_LOG_FACE_WARN("Received block is invalid or too large to process");
      return;
    }

  if (!this->decodeAndDispatchInput(element))
    {
      NFD_LOG_FACE_WARN("Received unrecognized TLV block of type " << element.type());
      // ignore unknown packet and proceed
    }
}

} // namespace nfd
