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

#ifndef NFD_DAEMON_FACE_WEBSOCKET_FACE_HPP
#define NFD_DAEMON_FACE_WEBSOCKET_FACE_HPP

#include "face.hpp"
#include "core/scheduler.hpp"

#ifndef HAVE_WEBSOCKET
#error "Cannot include this file when WebSocket support is not enabled"
#endif // HAVE_WEBSOCKET

#include "websocketpp.hpp"

namespace nfd {

namespace websocket {
typedef boost::asio::ip::tcp::endpoint Endpoint;
typedef websocketpp::server<websocketpp::config::asio> Server;
} // namespace websocket


/**
 * \brief Implementation of Face abstraction that uses WebSocket
 *        as underlying transport mechanism
 */
class WebSocketFace : public Face
{
public:
  WebSocketFace(const FaceUri& remoteUri, const FaceUri& localUri,
                websocketpp::connection_hdl hdl, websocket::Server& server);

  // from Face
  void
  sendInterest(const Interest& interest) DECL_OVERRIDE;

  void
  sendData(const Data& data) DECL_OVERRIDE;

  void
  close() DECL_OVERRIDE;

  void
  setPingEventId(scheduler::EventId& id)
  {
    m_pingEventId = id;
  }

protected:
  // friend because it needs to invoke protected handleReceive
  friend class WebSocketChannel;

  void
  handleReceive(const std::string& msg);

private:
  websocketpp::connection_hdl m_handle;
  websocket::Server& m_server;
  scheduler::EventId m_pingEventId;
  bool m_closed;
};

} // namespace nfd

#endif // NFD_DAEMON_FACE_WEBSOCKET_FACE_HPP
