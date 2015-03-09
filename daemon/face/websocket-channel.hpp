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

#ifndef NFD_DAEMON_FACE_WEBSOCKET_CHANNEL_HPP
#define NFD_DAEMON_FACE_WEBSOCKET_CHANNEL_HPP

#include "channel.hpp"
#include "websocket-face.hpp"

namespace nfd {

/**
 * \brief Class implementing WebSocket-based channel to create faces
 */
class WebSocketChannel : public Channel
{
public:
  /**
   * \brief Create WebSocket channel for the local endpoint
   *
   * To enable creation of faces upon incoming connections,
   * one needs to explicitly call WebSocketChannel::listen method.
   * The created socket is bound to the localEndpoint.
   *
   * \throw WebSocketChannel::Error if bind on the socket fails
   */
  explicit
  WebSocketChannel(const websocket::Endpoint& localEndpoint);

  /**
   * \brief Enable listening on the local endpoint, accept connections,
   *        and create faces when remote host makes a connection
   * \param onFaceCreated  Callback to notify successful creation of the face
   *
   * \throws WebSocketChannel::Error if called multiple times
   */
  void
  listen(const FaceCreatedCallback& onFaceCreated);

  /**
   * \brief Get number of faces in the channel
   */
  size_t
  size() const;

  bool
  isListening() const;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  setPingInterval(time::milliseconds interval);

  void
  setPongTimeout(time::milliseconds timeout);

private:
  void
  sendPing(websocketpp::connection_hdl hdl);

  void
  handlePong(websocketpp::connection_hdl hdl, std::string msg);

  void
  handlePongTimeout(websocketpp::connection_hdl hdl, std::string msg);

  void
  handleMessage(websocketpp::connection_hdl hdl, websocket::Server::message_ptr msg);

  void
  handleOpen(websocketpp::connection_hdl hdl);

  void
  handleClose(websocketpp::connection_hdl hdl);

private:
  websocket::Endpoint m_localEndpoint;
  websocket::Server m_server;

  std::map<websocketpp::connection_hdl, shared_ptr<WebSocketFace>,
           std::owner_less<websocketpp::connection_hdl>> m_channelFaces;

  /**
   * Callback for face creation
   */
  FaceCreatedCallback m_onFaceCreatedCallback;

  /**
   * \brief If true, it means the function listen has already been called
   */
  bool m_isListening;

  time::milliseconds m_pingInterval;
};

inline bool
WebSocketChannel::isListening() const
{
  return m_isListening;
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_WEBSOCKET_CHANNEL_HPP
