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

#ifndef NFD_DAEMON_FACE_UDP_CHANNEL_HPP
#define NFD_DAEMON_FACE_UDP_CHANNEL_HPP

#include "channel.hpp"
#include "udp-protocol.hpp"

#include <array>

namespace nfd {
namespace face {

/**
 * \brief Class implementing UDP-based channel to create faces
 */
class UdpChannel : public Channel
{
public:
  /**
   * \brief Create a UDP channel on the given \p localEndpoint
   *
   * To enable creation of faces upon incoming connections,
   * one needs to explicitly call UdpChannel::listen method.
   * The created socket is bound to \p localEndpoint.
   */
  UdpChannel(const udp::Endpoint& localEndpoint,
             time::nanoseconds idleTimeout,
             bool wantCongestionMarking);

  bool
  isListening() const override
  {
    return m_socket.is_open();
  }

  size_t
  size() const override
  {
    return m_channelFaces.size();
  }

  /**
   * \brief Create a unicast UDP face toward \p remoteEndpoint
   */
  void
  connect(const udp::Endpoint& remoteEndpoint,
          const FaceParams& params,
          const FaceCreatedCallback& onFaceCreated,
          const FaceCreationFailedCallback& onConnectFailed);

  /**
   * \brief Start listening
   *
   * Enable listening on the local endpoint, waiting for incoming datagrams,
   * and creating a face when a datagram is received from a new remote host.
   *
   * Faces created in this way will have on-demand persistency.
   *
   * \param onFaceCreated Callback to notify successful creation of a face
   * \param onFaceCreationFailed Callback to notify errors
   */
  void
  listen(const FaceCreatedCallback& onFaceCreated,
         const FaceCreationFailedCallback& onFaceCreationFailed);

private:
  void
  waitForNewPeer(const FaceCreatedCallback& onFaceCreated,
                 const FaceCreationFailedCallback& onReceiveFailed);

  /**
   * \brief The channel has received a packet from a remote
   *        endpoint not associated with any UDP face yet
   */
  void
  handleNewPeer(const boost::system::error_code& error,
                size_t nBytesReceived,
                const FaceCreatedCallback& onFaceCreated,
                const FaceCreationFailedCallback& onReceiveFailed);

  std::pair<bool, shared_ptr<Face>>
  createFace(const udp::Endpoint& remoteEndpoint,
             const FaceParams& params);

private:
  const udp::Endpoint m_localEndpoint;
  udp::Endpoint m_remoteEndpoint; ///< The latest peer that started communicating with us
  boost::asio::ip::udp::socket m_socket; ///< Socket used to "accept" new peers
  std::array<uint8_t, ndn::MAX_NDN_PACKET_SIZE> m_receiveBuffer;
  std::map<udp::Endpoint, shared_ptr<Face>> m_channelFaces;
  const time::nanoseconds m_idleFaceTimeout; ///< Timeout for automatic closure of idle on-demand faces
  bool m_wantCongestionMarking;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_UDP_CHANNEL_HPP
