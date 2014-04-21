/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology,
 *                     The University of Memphis
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
 **/

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
}


void
WebSocketFace::sendInterest(const Interest& interest)
{
  this->onSendInterest(interest);
  const Block& payload = interest.wireEncode();
  m_server.send(m_handle, payload.wire(), payload.size(), websocketpp::frame::opcode::binary);
}

void
WebSocketFace::sendData(const Data& data)
{
  this->onSendData(data);
  const Block& payload = data.wireEncode();
  m_server.send(m_handle, payload.wire(), payload.size(), websocketpp::frame::opcode::binary);
}

void
WebSocketFace::close()
{
  if (m_closed == false)
    {
      m_closed = true;
      websocketpp::lib::error_code ecode;
      m_server.close(m_handle, websocketpp::close::status::normal, "closed by nfd", ecode);
    }
}

void
WebSocketFace::handleReceive(const std::string& msg)
{
  // Copy message into Face internal buffer
  BOOST_ASSERT(msg.size() <= MAX_NDN_PACKET_SIZE);

  // Try to parse message data
  bool isOk = true;
  Block element;
  isOk = Block::fromBuffer(reinterpret_cast<const uint8_t*>(msg.c_str()), msg.size(), element);
  if (!isOk)
    {
      NFD_LOG_TRACE("[id:" << this->getId()
                    << "] Received invalid NDN packet of length ["
                    << msg.size() << "]");
      return;
    }

  if (!this->decodeAndDispatchInput(element))
    {
      NFD_LOG_WARN("[id:" << this->getId()
                   << "] Received unrecognized block of type ["
                   << element.type() << "]");
      // ignore unknown packet and proceed
    }
}

} // namespace nfd
