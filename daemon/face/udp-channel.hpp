/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

namespace nfd {

namespace udp {
typedef boost::asio::ip::udp::endpoint Endpoint;
} // namespace udp

class UdpFace;

/**
 * \brief Class implementing UDP-based channel to create faces
 */
class UdpChannel : public Channel
{
public:
  /**
   * \brief Create UDP channel for the local endpoint
   *
   * To enable creation of faces upon incoming connections,
   * one needs to explicitly call UdpChannel::listen method.
   * The created socket is bound to the localEndpoint.
   * reuse_address option is set
   *
   * \throw UdpChannel::Error if bind on the socket fails
   */
  UdpChannel(const udp::Endpoint& localEndpoint,
             const time::seconds& timeout);

  /**
   * \brief Enable listening on the local endpoint, accept connections,
   *        and create faces when remote host makes a connection
   * \param onFaceCreated  Callback to notify successful creation of the face
   * \param onAcceptFailed Callback to notify when channel fails
   *
   * Once a face is created, if it doesn't send/receive anything for
   * a period of time equal to timeout, it will be destroyed
   * \todo this functionality has to be implemented
   *
   * \throws UdpChannel::Error if called multiple times
   */
  void
  listen(const FaceCreatedCallback& onFaceCreated,
         const ConnectFailedCallback& onReceiveFailed);

  /**
   * \brief Create a face by establishing connection to remote endpoint
   *
   * \throw UdpChannel::Error if bind or connect on the socket fail
   */
  void
  connect(const udp::Endpoint& remoteEndpoint,
          ndn::nfd::FacePersistency persistency,
          const FaceCreatedCallback& onFaceCreated,
          const ConnectFailedCallback& onConnectFailed);

  /**
   * \brief Get number of faces in the channel
   */
  size_t
  size() const;

  bool
  isListening() const;

private:
  std::pair<bool, shared_ptr<UdpFace>>
  createFace(const udp::Endpoint& remoteEndpoint, ndn::nfd::FacePersistency persistency);

  /**
   * \brief The channel has received a new packet from a remote
   *        endpoint that is not associated with any UdpFace yet
   */
  void
  handleNewPeer(const boost::system::error_code& error,
                size_t nBytesReceived,
                const FaceCreatedCallback& onFaceCreated,
                const ConnectFailedCallback& onReceiveFailed);

private:
  std::map<udp::Endpoint, shared_ptr<UdpFace>> m_channelFaces;

  udp::Endpoint m_localEndpoint;

  /**
   * \brief The latest peer that started communicating with us
   */
  udp::Endpoint m_remoteEndpoint;

  /**
   * \brief Socket used to "accept" new communication
   */
  boost::asio::ip::udp::socket m_socket;

  /**
   * \brief When this timeout expires, all idle on-demand faces will be closed
   */
  time::seconds m_idleFaceTimeout;

  uint8_t m_inputBuffer[ndn::MAX_NDN_PACKET_SIZE];
};

inline bool
UdpChannel::isListening() const
{
  return m_socket.is_open();
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_UDP_CHANNEL_HPP
