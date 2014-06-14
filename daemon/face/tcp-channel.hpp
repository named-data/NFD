/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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

#ifndef NFD_DAEMON_FACE_TCP_CHANNEL_HPP
#define NFD_DAEMON_FACE_TCP_CHANNEL_HPP

#include "channel.hpp"
#include "tcp-face.hpp"
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

  virtual
  ~TcpChannel();

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
   * \brief Create a face by establishing connection to the specified
   *        remote host and remote port
   *
   * This method will never block and will return immediately. All
   * necessary hostname and port resolution and connection will happen
   * in asynchronous mode.
   *
   * If connection cannot be established within specified timeout, it
   * will be aborted.
   */
  void
  connect(const std::string& remoteHost, const std::string& remotePort,
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
  createFace(const shared_ptr<boost::asio::ip::tcp::socket>& socket,
             const FaceCreatedCallback& onFaceCreated,
             bool isOnDemand);

  void
  afterFaceFailed(tcp::Endpoint &endpoint);

  void
  handleSuccessfulAccept(const boost::system::error_code& error,
                         const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                         const FaceCreatedCallback& onFaceCreated,
                         const ConnectFailedCallback& onConnectFailed);

  void
  handleSuccessfulConnect(const boost::system::error_code& error,
                          const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                          const EventId& connectTimeoutEvent,
                          const FaceCreatedCallback& onFaceCreated,
                          const ConnectFailedCallback& onConnectFailed);

  void
  handleFailedConnect(const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                      const ConnectFailedCallback& onConnectFailed);

  void
  handleEndpointResolution(const boost::system::error_code& error,
                           boost::asio::ip::tcp::resolver::iterator remoteEndpoint,
                           const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                           const EventId& connectTimeoutEvent,
                           const FaceCreatedCallback& onFaceCreated,
                           const ConnectFailedCallback& onConnectFailed,
                           const shared_ptr<boost::asio::ip::tcp::resolver>& resolver);

private:
  tcp::Endpoint m_localEndpoint;

  typedef std::map< tcp::Endpoint, shared_ptr<Face> > ChannelFaceMap;
  ChannelFaceMap m_channelFaces;

  bool m_isListening;
  shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
};

inline bool
TcpChannel::isListening() const
{
  return m_isListening;
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_TCP_CHANNEL_HPP
