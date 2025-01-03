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

#ifndef NFD_DAEMON_FACE_UNIX_STREAM_CHANNEL_HPP
#define NFD_DAEMON_FACE_UNIX_STREAM_CHANNEL_HPP

#include "channel.hpp"

#include <boost/asio/local/stream_protocol.hpp>

namespace nfd::unix_stream {
using Endpoint = boost::asio::local::stream_protocol::endpoint;
} // namespace nfd::unix_stream

namespace nfd::face {

/**
 * \brief Class implementing a local channel to create faces.
 *
 * Channel can create faces as a response to incoming IPC connections
 * (UnixStreamChannel::listen() needs to be called for that to work).
 */
class UnixStreamChannel final : public Channel
{
public:
  /**
   * \brief Create a UnixStream channel for the specified \p endpoint.
   *
   * To enable the creation of faces upon incoming connections, one needs to
   * explicitly call listen().
   */
  UnixStreamChannel(const unix_stream::Endpoint& endpoint, bool wantCongestionMarking);

  ~UnixStreamChannel() final;

  bool
  isListening() const final
  {
    return m_isListening;
  }

  size_t
  size() const final
  {
    return m_size;
  }

  /**
   * \brief Start listening.
   *
   * Enable listening on the Unix socket, waiting for incoming connections,
   * and creating a face when a connection is made.
   *
   * Faces created in this way will have on-demand persistency.
   *
   * \param onFaceCreated  Callback to notify successful creation of the face
   * \param onAcceptFailed Callback to notify when channel fails (accept call
   *                       returns an error)
   * \param backlog        The maximum length of the queue of pending incoming
   *                       connections
   * \throw std::system_error
   */
  void
  listen(const FaceCreatedCallback& onFaceCreated,
         const FaceCreationFailedCallback& onAcceptFailed,
         int backlog = boost::asio::socket_base::max_listen_connections);

private:
  void
  accept(const FaceCreatedCallback& onFaceCreated,
         const FaceCreationFailedCallback& onAcceptFailed);

private:
  const unix_stream::Endpoint m_endpoint;
  const bool m_wantCongestionMarking;
  bool m_isListening = false;
  boost::asio::local::stream_protocol::acceptor m_acceptor;
  size_t m_size = 0;
};

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_UNIX_STREAM_CHANNEL_HPP
