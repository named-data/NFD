/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_TCP_CHANNEL_HPP
#define NFD_FACE_TCP_CHANNEL_HPP

#include "channel.hpp"
#include "core/time.hpp"
#include <ndn-cpp-dev/util/monotonic_deadline_timer.hpp>
#include "tcp-face.hpp"

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
          const time::Duration& timeout = time::seconds(4));

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
          const time::Duration& timeout = time::seconds(4));

  /**
   * \brief Get number of faces in the channel
   */
  size_t
  size() const;

private:
  void
  createFace(const shared_ptr<boost::asio::ip::tcp::socket>& socket,
             const FaceCreatedCallback& onFaceCreated);

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
                          const shared_ptr<boost::asio::monotonic_deadline_timer>& timer,
                          const FaceCreatedCallback& onFaceCreated,
                          const ConnectFailedCallback& onConnectFailed);

  void
  handleFailedConnect(const boost::system::error_code& error,
                      const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                      const shared_ptr<boost::asio::monotonic_deadline_timer>& timer,
                      const ConnectFailedCallback& onConnectFailed);

  void
  handleEndpointResolution(const boost::system::error_code& error,
                           boost::asio::ip::tcp::resolver::iterator remoteEndpoint,
                           const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                           const shared_ptr<boost::asio::monotonic_deadline_timer>& timer,
                           const FaceCreatedCallback& onFaceCreated,
                           const ConnectFailedCallback& onConnectFailed,
                           const shared_ptr<boost::asio::ip::tcp::resolver>& resolver);

private:
  tcp::Endpoint m_localEndpoint;

  typedef std::map< tcp::Endpoint, shared_ptr<Face> > ChannelFaceMap;
  ChannelFaceMap m_channelFaces;

  bool isListening;
  shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
};

} // namespace nfd

#endif // NFD_FACE_TCP_CHANNEL_HPP
