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

#ifndef NFD_DAEMON_FACE_TCP_CHANNEL_HPP
#define NFD_DAEMON_FACE_TCP_CHANNEL_HPP

#include "channel.hpp"
#include "core/scheduler.hpp"

namespace nfd {

namespace tcp {
typedef boost::asio::ip::tcp::endpoint Endpoint;
} // namespace tcp

/**
 * \brief Class implementing TCP-based channel to create faces
 *
 * Channel can create faces as a response to incoming TCP
 * connections (TcpChannel::listen needs to be called for that
 * to work) or explicitly after using TcpChannel::connect method.
 */
class TcpChannel : public Channel
{
public:
  /**
   * \brief Create TCP channel for the local endpoint
   *
   * To enable creation faces upon incoming connections,
   * one needs to explicitly call TcpChannel::listen method.
   */
  explicit
  TcpChannel(const tcp::Endpoint& localEndpoint);

  /**
   * \brief Enable listening on the local endpoint, accept connections,
   *        and create faces when remote host makes a connection
   * \param onFaceCreated  Callback to notify successful creation of the face
   * \param onAcceptFailed Callback to notify when channel fails (accept call
   *                       returns an error)
   * \param backlog        The maximum length of the queue of pending incoming
   *                       connections
   */
  void
  listen(const FaceCreatedCallback& onFaceCreated,
         const ConnectFailedCallback& onAcceptFailed,
         int backlog = boost::asio::ip::tcp::acceptor::max_connections);

  /**
   * \brief Create a face by establishing connection to remote endpoint
   */
  void
  connect(const tcp::Endpoint& remoteEndpoint,
          const FaceCreatedCallback& onFaceCreated,
          const ConnectFailedCallback& onConnectFailed,
          const time::seconds& timeout = time::seconds(4));

  /**
   * \brief Get number of faces in the channel
   */
  size_t
  size() const;

  bool
  isListening() const;

private:
  void
  createFace(boost::asio::ip::tcp::socket socket,
             const FaceCreatedCallback& onFaceCreated,
             bool isOnDemand);

  void
  accept(const FaceCreatedCallback& onFaceCreated,
         const ConnectFailedCallback& onConnectFailed);

  void
  handleAccept(const boost::system::error_code& error,
               const FaceCreatedCallback& onFaceCreated,
               const ConnectFailedCallback& onConnectFailed);

  void
  handleConnect(const boost::system::error_code& error,
                const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                const scheduler::EventId& connectTimeoutEvent,
                const FaceCreatedCallback& onFaceCreated,
                const ConnectFailedCallback& onConnectFailed);

  void
  handleConnectTimeout(const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                       const ConnectFailedCallback& onConnectFailed);

private:
  std::map<tcp::Endpoint, shared_ptr<Face>> m_channelFaces;

  tcp::Endpoint m_localEndpoint;
  boost::asio::ip::tcp::acceptor m_acceptor;
  boost::asio::ip::tcp::socket m_acceptSocket;
};

inline bool
TcpChannel::isListening() const
{
  return m_acceptor.is_open();
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_TCP_CHANNEL_HPP
