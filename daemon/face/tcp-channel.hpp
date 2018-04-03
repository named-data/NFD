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

#ifndef NFD_DAEMON_FACE_TCP_CHANNEL_HPP
#define NFD_DAEMON_FACE_TCP_CHANNEL_HPP

#include "channel.hpp"
#include "core/scheduler.hpp"

namespace nfd {

namespace tcp {
typedef boost::asio::ip::tcp::endpoint Endpoint;
} // namespace tcp

namespace face {

using DetermineFaceScopeFromAddress = std::function<ndn::nfd::FaceScope(const boost::asio::ip::address& local,
                                                                        const boost::asio::ip::address& remote)>;

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
  TcpChannel(const tcp::Endpoint& localEndpoint, bool wantCongestionMarking,
             DetermineFaceScopeFromAddress determineFaceScope);

  bool
  isListening() const override
  {
    return m_acceptor.is_open();
  }

  size_t
  size() const override
  {
    return m_channelFaces.size();
  }

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
         const FaceCreationFailedCallback& onAcceptFailed,
         int backlog = boost::asio::ip::tcp::acceptor::max_connections);

  /**
   * \brief Create a face by establishing a TCP connection to \p remoteEndpoint
   */
  void
  connect(const tcp::Endpoint& remoteEndpoint,
          const FaceParams& params,
          const FaceCreatedCallback& onFaceCreated,
          const FaceCreationFailedCallback& onConnectFailed,
          time::nanoseconds timeout = 8_s);

private:
  void
  createFace(boost::asio::ip::tcp::socket&& socket,
             const FaceParams& params,
             const FaceCreatedCallback& onFaceCreated);

  void
  accept(const FaceCreatedCallback& onFaceCreated,
         const FaceCreationFailedCallback& onAcceptFailed);

  void
  handleAccept(const boost::system::error_code& error,
               const FaceCreatedCallback& onFaceCreated,
               const FaceCreationFailedCallback& onAcceptFailed);

  void
  handleConnect(const boost::system::error_code& error,
                const tcp::Endpoint& remoteEndpoint,
                const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                const FaceParams& params,
                const scheduler::EventId& connectTimeoutEvent,
                const FaceCreatedCallback& onFaceCreated,
                const FaceCreationFailedCallback& onConnectFailed);

  void
  handleConnectTimeout(const tcp::Endpoint& remoteEndpoint,
                       const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                       const FaceCreationFailedCallback& onConnectFailed);

private:
  const tcp::Endpoint m_localEndpoint;
  boost::asio::ip::tcp::acceptor m_acceptor;
  boost::asio::ip::tcp::socket m_socket;
  std::map<tcp::Endpoint, shared_ptr<Face>> m_channelFaces;
  bool m_wantCongestionMarking;
  DetermineFaceScopeFromAddress m_determineFaceScope;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_TCP_CHANNEL_HPP
