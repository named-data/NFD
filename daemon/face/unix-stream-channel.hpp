/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_UNIX_STREAM_CHANNEL_HPP
#define NFD_FACE_UNIX_STREAM_CHANNEL_HPP

#include "common.hpp"
#include "unix-stream-face.hpp"

namespace nfd {

namespace unix_stream {
  typedef boost::asio::local::stream_protocol::endpoint Endpoint;
} // namespace unix_stream

/**
 * \brief Class implementing a local channel to create faces
 *
 * Channel can create faces as a response to incoming IPC connections
 * (UnixStreamChannel::listen needs to be called for that to work).
 */
class UnixStreamChannel // : protected SessionBasedChannel
{
public:
  /**
   * \brief Prototype for the callback called when a face is created
   *        (as a response to an incoming connection)
   */
  typedef function<void(const shared_ptr<UnixStreamFace>& newFace)> FaceCreatedCallback;

  /**
   * \brief Prototype for the callback that is called when a face
   *        fails to be created
   */
  typedef function<void(const std::string& reason)> ConnectFailedCallback;

  /**
   * \brief Create UnixStream channel for the specified endpoint
   *
   * To enable creation of faces upon incoming connections, one
   * needs to explicitly call UnixStreamChannel::listen method.
   */
  UnixStreamChannel(boost::asio::io_service& ioService,
                    const unix_stream::Endpoint& endpoint);

  ~UnixStreamChannel();

  /**
   * \brief Enable listening on the local endpoint, accept connections,
   *        and create faces when a connection is made
   * \param onFaceCreated  Callback to notify successful creation of the face
   * \param onAcceptFailed Callback to notify when channel fails (accept call
   *                       returns an error)
   * \param backlog        The maximum length of the queue of pending incoming
   *                       connections
   */
  void
  listen(const FaceCreatedCallback& onFaceCreated,
         const ConnectFailedCallback& onAcceptFailed,
         int backlog = boost::asio::local::stream_protocol::acceptor::max_connections);

private:
  void
  createFace(const shared_ptr<boost::asio::local::stream_protocol::socket>& socket,
             const FaceCreatedCallback& onFaceCreated);

  void
  handleSuccessfulAccept(const boost::system::error_code& error,
                         const shared_ptr<boost::asio::local::stream_protocol::socket>& socket,
                         const FaceCreatedCallback& onFaceCreated,
                         const ConnectFailedCallback& onConnectFailed);

private:
  boost::asio::io_service& m_ioService;
  unix_stream::Endpoint m_endpoint;
  shared_ptr<boost::asio::local::stream_protocol::acceptor> m_acceptor;
};

} // namespace nfd

#endif // NFD_FACE_UNIX_STREAM_CHANNEL_HPP
