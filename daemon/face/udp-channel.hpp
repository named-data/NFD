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
#include "core/global-io.hpp"
#include "udp-face.hpp"

namespace nfd {

namespace udp {
typedef boost::asio::ip::udp::endpoint Endpoint;
} // namespace udp

/**
 * \brief Class implementing UDP-based channel to create faces
 *
 *
 */
class UdpChannel : public Channel
{
public:
  /**
   * \brief Exception of UdpChannel
   */
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : runtime_error(what) {}
  };

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
         const ConnectFailedCallback& onAcceptFailed);

  /**
   * \brief Create a face by establishing connection to remote endpoint
   *
   * \throw UdpChannel::Error if bind or connect on the socket fail
   */
  void
  connect(const udp::Endpoint& remoteEndpoint,
          const FaceCreatedCallback& onFaceCreated,
          const ConnectFailedCallback& onConnectFailed);
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
          const ConnectFailedCallback& onConnectFailed);

  /**
   * \brief Get number of faces in the channel
   */
  size_t
  size() const;

private:
  shared_ptr<UdpFace>
  createFace(const shared_ptr<boost::asio::ip::udp::socket>& socket,
             const FaceCreatedCallback& onFaceCreated,
             bool isOnDemand);
  void
  afterFaceFailed(udp::Endpoint& endpoint);

  /**
   * \brief The UdpChannel has received a new pkt from a remote endpoint not yet
   *        associated with any UdpFace
   */
  void
  newPeer(const boost::system::error_code& error, size_t nBytesReceived);

  void
  handleEndpointResolution(const boost::system::error_code& error,
                           boost::asio::ip::udp::resolver::iterator remoteEndpoint,
                           const FaceCreatedCallback& onFaceCreated,
                           const ConnectFailedCallback& onConnectFailed,
                           const shared_ptr<boost::asio::ip::udp::resolver>& resolver);

private:
  udp::Endpoint m_localEndpoint;

  /**
   * \brief Endpoint used to store the information about the last new remote endpoint
   */
  udp::Endpoint m_newRemoteEndpoint;

  /**
   * Callbacks for face creation.
   * New communications are detected using async_receive_from.
   * Its handler has a fixed signature. No space for the face callback
   */
  FaceCreatedCallback onFaceCreatedNewPeerCallback;

  // @todo remove the onConnectFailedNewPeerCallback if it remains unused
  ConnectFailedCallback onConnectFailedNewPeerCallback;

  /**
   * \brief Socket used to "accept" new communication
   **/
  shared_ptr<boost::asio::ip::udp::socket> m_socket;

  uint8_t m_inputBuffer[ndn::MAX_NDN_PACKET_SIZE];

  typedef std::map< udp::Endpoint, shared_ptr<UdpFace> > ChannelFaceMap;
  ChannelFaceMap m_channelFaces;

  /**
   * \brief If true, it means the function listen has already been called
   */
  bool m_isListening;

  /**
   * \brief every time m_idleFaceTimeout expires all the idle (and on-demand)
   *        faces will be removed
   */
  time::seconds m_idleFaceTimeout;
};

} // namespace nfd

#endif // NFD_DAEMON_FACE_UDP_CHANNEL_HPP
