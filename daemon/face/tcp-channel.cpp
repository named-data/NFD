/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "tcp-channel.hpp"

namespace nfd {

using namespace boost::asio;

TcpChannel::TcpChannel(io_service& ioService,
                       const tcp::Endpoint& localEndpoint)
  : m_ioService(ioService)
  , m_localEndpoint(localEndpoint)
{
}

void
TcpChannel::listen(const FaceCreatedCallback& onFaceCreated,
                   const ConnectFailedCallback& onAcceptFailed,
                   int backlog/* = tcp::acceptor::max_connections*/)
{
  m_acceptor = make_shared<ip::tcp::acceptor>(boost::ref(m_ioService));
  m_acceptor->open(m_localEndpoint.protocol());
  m_acceptor->set_option(ip::tcp::acceptor::reuse_address(true));
  m_acceptor->bind(m_localEndpoint);
  m_acceptor->listen(backlog);

  shared_ptr<ip::tcp::socket> clientSocket =
    make_shared<ip::tcp::socket>(boost::ref(m_ioService));
  m_acceptor->async_accept(*clientSocket,
                           bind(&TcpChannel::handleConnection, this, _1,
                                clientSocket,
                                onFaceCreated, onAcceptFailed));
}

void
TcpChannel::connect(const tcp::Endpoint& remoteEndpoint,
                    const TcpChannel::FaceCreatedCallback& onFaceCreated,
                    const TcpChannel::ConnectFailedCallback& onConnectFailed,
                    const time::Duration& timeout/* = time::seconds(4)*/)
{
  ChannelFaceMap::iterator i = m_channelFaces.find(remoteEndpoint);
  if (i != m_channelFaces.end()) {
    onFaceCreated(i->second);
    return;
  }

  shared_ptr<ip::tcp::socket> clientSocket =
    make_shared<ip::tcp::socket>(boost::ref(m_ioService));

  shared_ptr<monotonic_deadline_timer> connectTimeoutTimer =
    make_shared<monotonic_deadline_timer>(boost::ref(m_ioService));

  // not sure if it works. This will bind to something...
  // Do we need reuse here too?
  clientSocket->bind(m_localEndpoint);

  clientSocket->async_connect(remoteEndpoint,
                              bind(&TcpChannel::handleSuccessfulConnect, this, _1,
                                   clientSocket, connectTimeoutTimer,
                                   onFaceCreated, onConnectFailed));

  connectTimeoutTimer->expires_from_now(timeout);
  connectTimeoutTimer->async_wait(bind(&TcpChannel::handleFailedConnect, this, _1,
                                       clientSocket, connectTimeoutTimer,
                                       onConnectFailed));
}

void
TcpChannel::connect(const std::string& remoteHost, const std::string& remotePort,
                    const TcpChannel::FaceCreatedCallback& onFaceCreated,
                    const TcpChannel::ConnectFailedCallback& onConnectFailed,
                    const time::Duration& timeout/* = time::seconds(4)*/)
{
  shared_ptr<ip::tcp::socket> clientSocket =
    make_shared<ip::tcp::socket>(boost::ref(m_ioService));

  shared_ptr<monotonic_deadline_timer> connectTimeoutTimer =
    make_shared<monotonic_deadline_timer>(boost::ref(m_ioService));

  // not sure if it works. This will bind to something...
  // Do we need reuse here too?
  clientSocket->bind(m_localEndpoint);

  ip::tcp::resolver::query query(remoteHost, remotePort);
  shared_ptr<ip::tcp::resolver> resolver =
    make_shared<ip::tcp::resolver>(boost::ref(m_ioService));

  resolver->async_resolve(query,
                          bind(&TcpChannel::handleEndpointResoution, this, _1, _2,
                               clientSocket, connectTimeoutTimer,
                               onFaceCreated, onConnectFailed));

  connectTimeoutTimer->expires_from_now(timeout);
  connectTimeoutTimer->async_wait(bind(&TcpChannel::handleFailedConnect, this, _1,
                                       clientSocket, connectTimeoutTimer,
                                       onConnectFailed));
}


void
TcpChannel::handleConnection(const boost::system::error_code& error,
                             const shared_ptr<ip::tcp::socket>& socket,
                             const FaceCreatedCallback& onFaceCreated,
                             const ConnectFailedCallback& onConnectFailed)
{
  if (error) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    /// \todo Log the error
    onConnectFailed("Connect to remote endpoint failed: " +
                    error.category().message(error.value()));
    return;
  }

  /**
   * \todo Remove FaceId from here
   */
  shared_ptr<TcpFace> face = make_shared<TcpFace>(1, boost::cref(socket));
  onFaceCreated(face);

  tcp::Endpoint remoteEndpoint = socket->remote_endpoint();
  m_channelFaces[remoteEndpoint] = face;
}

void
TcpChannel::handleSuccessfulConnect(const boost::system::error_code& error,
                                    const shared_ptr<ip::tcp::socket>& socket,
                                    const shared_ptr<monotonic_deadline_timer>& timer,
                                    const FaceCreatedCallback& onFaceCreated,
                                    const ConnectFailedCallback& onConnectFailed)
{
  timer->cancel();

  if (error) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    socket->close();
    onConnectFailed("Connect to remote endpoint failed: " +
                    error.category().message(error.value()));
    return;
  }

  handleConnection(error, socket, onFaceCreated, onConnectFailed);
}

void
TcpChannel::handleFailedConnect(const boost::system::error_code& error,
                                const shared_ptr<ip::tcp::socket>& socket,
                                const shared_ptr<monotonic_deadline_timer>& timer,
                                const ConnectFailedCallback& onConnectFailed)
{
  if (error) { // e.g., cancelled
    return;
  }

  onConnectFailed("Connect to remote endpoint timed out: " +
                  error.category().message(error.value()));
  socket->close(); // abort the connection
}

void
TcpChannel::handleEndpointResoution(const boost::system::error_code& error,
                                    ip::tcp::resolver::iterator remoteEndpoint,
                                    const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                                    const shared_ptr<boost::asio::monotonic_deadline_timer>& timer,
                                    const FaceCreatedCallback& onFaceCreated,
                                    const ConnectFailedCallback& onConnectFailed)
{
  if (error ||
      remoteEndpoint == ip::tcp::resolver::iterator())
    {
      if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
        return;

      socket->close();
      timer->cancel();
      onConnectFailed("Remote endpoint hostname or port cannot be resolved: " +
                      error.category().message(error.value()));
      return;
    }

  // got endpoint, now trying to connect (only try the first resolution option)
  socket->async_connect(*remoteEndpoint,
                        bind(&TcpChannel::handleSuccessfulConnect, this, _1,
                             socket, timer,
                             onFaceCreated, onConnectFailed));
}


} // namespace nfd
