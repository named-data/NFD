/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_UDP_CHANNEL_HPP
#define NFD_FACE_UDP_CHANNEL_HPP

#include "channel.hpp"
#include "core/global-io.hpp"
#include "core/scheduler.hpp"
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

  virtual
  ~UdpChannel();

  /**
   * \brief Enable listening on the local endpoint, accept connections,
   *        and create faces when remote host makes a connection
   * \param onFaceCreated  Callback to notify successful creation of the face
   * \param onAcceptFailed Callback to notify when channel fails
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
          const FaceCreatedCallback& onFaceCreated);
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
  newPeer(const boost::system::error_code& error,
                   std::size_t nBytesReceived);

  void
  handleEndpointResolution(const boost::system::error_code& error,
                           boost::asio::ip::udp::resolver::iterator remoteEndpoint,
                           const FaceCreatedCallback& onFaceCreated,
                           const ConnectFailedCallback& onConnectFailed,
                           const shared_ptr<boost::asio::ip::udp::resolver>& resolver);

  void
  closeIdleFaces();

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

  uint8_t m_inputBuffer[MAX_NDN_PACKET_SIZE];

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

  EventId m_closeIdleFaceEvent;

};

} // namespace nfd

#endif // NFD_FACE_UDP_CHANNEL_HPP
