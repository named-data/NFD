/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_WEBSOCKET_CHANNEL_HPP
#define NFD_DAEMON_FACE_WEBSOCKET_CHANNEL_HPP

#include "channel.hpp"
#include "websocketpp.hpp"

#include <map>

namespace nfd::websocket {
using Endpoint = boost::asio::ip::tcp::endpoint;
} // namespace nfd::websocket

namespace nfd::face {

/**
 * \brief Class implementing a WebSocket-based channel to create faces.
 */
class WebSocketChannel final : public Channel
{
public:
  /**
   * \brief Create a WebSocket channel for the given \p localEndpoint.
   *
   * To enable the creation of faces upon incoming connections, one needs to
   * explicitly call listen(). The created channel is bound to \p localEndpoint.
   */
  explicit
  WebSocketChannel(const websocket::Endpoint& localEndpoint);

  bool
  isListening() const final
  {
    return m_server.is_listening();
  }

  size_t
  size() const final
  {
    return m_channelFaces.size();
  }

  /**
   * \brief Enable listening on the local endpoint, accept connections,
   *        and create faces when remote host makes a connection.
   *
   * \param onFaceCreated Callback to notify successful creation of a face
   */
  void
  listen(const FaceCreatedCallback& onFaceCreated);

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /**
   * \pre listen() has not been invoked.
   */
  void
  setPingInterval(time::milliseconds interval);

  /**
   * \pre listen() has not been invoked.
   */
  void
  setPongTimeout(time::milliseconds timeout);

  void
  handlePong(websocketpp::connection_hdl hdl);

  void
  handlePongTimeout(websocketpp::connection_hdl hdl);

private:
  void
  handleMessage(websocketpp::connection_hdl hdl,
                websocket::Server::message_ptr msg);

  void
  handleOpen(websocketpp::connection_hdl hdl);

  void
  handleClose(websocketpp::connection_hdl hdl);

private:
  const websocket::Endpoint m_localEndpoint;
  websocket::Server m_server;
  std::map<websocketpp::connection_hdl, shared_ptr<Face>,
           std::owner_less<websocketpp::connection_hdl>> m_channelFaces;
  FaceCreatedCallback m_onFaceCreatedCallback;
  time::milliseconds m_pingInterval;
};

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_WEBSOCKET_CHANNEL_HPP
