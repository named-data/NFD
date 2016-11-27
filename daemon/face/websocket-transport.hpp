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

#ifndef NFD_DAEMON_FACE_WEBSOCKET_TRANSPORT_HPP
#define NFD_DAEMON_FACE_WEBSOCKET_TRANSPORT_HPP

#include "transport.hpp"
#include "websocketpp.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace face {

/** \brief counters provided by WebSocketTransport
 *  \note The type name 'WebSocketTransportCounters' is implementation detail.
 *        Use 'WebSocketTransport::Counters' in public API.
 */
class WebSocketTransportCounters : public virtual Transport::Counters
{
public:
  /** \brief count of outgoing Pings
   */
  PacketCounter nOutPings;

  /** \brief count of incoming Pongs
   */
  PacketCounter nInPongs;
};

/** \brief A Transport that communicates on a WebSocket connection
 */
class WebSocketTransport final : public Transport
                               , protected virtual WebSocketTransportCounters
{
public:
  /** \brief counters provided by WebSocketTransport
   */
  typedef WebSocketTransportCounters Counters;

  WebSocketTransport(websocketpp::connection_hdl hdl,
                     websocket::Server& server,
                     time::milliseconds pingInterval);

  const Counters&
  getCounters() const final;

  /** \brief Translates a message into a Block
   *         and delivers it to the link service
   */
  void
  receiveMessage(const std::string& msg);

  void
  handlePong();

  void
  handlePongTimeout();

protected:
  void
  doClose() final;

private:
  void
  doSend(Transport::Packet&& packet) final;

  void
  schedulePing();

  void
  sendPing();

  void
  processErrorCode(const websocketpp::lib::error_code& error);

private:
  websocketpp::connection_hdl m_handle;
  websocket::Server& m_server;
  time::milliseconds m_pingInterval;
  scheduler::ScopedEventId m_pingEventId;
};

inline const WebSocketTransport::Counters&
WebSocketTransport::getCounters() const
{
  return *this;
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_WEBSOCKET_TRANSPORT_HPP
