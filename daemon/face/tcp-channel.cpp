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

#include "tcp-channel.hpp"
#include "core/global-io.hpp"

namespace nfd {

NFD_LOG_INIT("TcpChannel");

using namespace boost::asio;

TcpChannel::TcpChannel(const tcp::Endpoint& localEndpoint)
  : m_localEndpoint(localEndpoint)
  , m_isListening(false)
{
  this->setUri(FaceUri(localEndpoint));
}

TcpChannel::~TcpChannel()
{
}

void
TcpChannel::listen(const FaceCreatedCallback& onFaceCreated,
                   const ConnectFailedCallback& onAcceptFailed,
                   int backlog/* = tcp::acceptor::max_connections*/)
{
  m_acceptor = make_shared<ip::tcp::acceptor>(ref(getGlobalIoService()));
  m_acceptor->open(m_localEndpoint.protocol());
  m_acceptor->set_option(ip::tcp::acceptor::reuse_address(true));
  if (m_localEndpoint.address().is_v6())
    {
      m_acceptor->set_option(ip::v6_only(true));
    }
  m_acceptor->bind(m_localEndpoint);
  m_acceptor->listen(backlog);

  shared_ptr<ip::tcp::socket> clientSocket =
    make_shared<ip::tcp::socket>(ref(getGlobalIoService()));
  m_acceptor->async_accept(*clientSocket,
                           bind(&TcpChannel::handleSuccessfulAccept, this, _1,
                                clientSocket,
                                onFaceCreated, onAcceptFailed));

  m_isListening = true;
}

void
TcpChannel::connect(const tcp::Endpoint& remoteEndpoint,
                    const TcpChannel::FaceCreatedCallback& onFaceCreated,
                    const TcpChannel::ConnectFailedCallback& onConnectFailed,
                    const time::seconds& timeout/* = time::seconds(4)*/)
{
  ChannelFaceMap::iterator i = m_channelFaces.find(remoteEndpoint);
  if (i != m_channelFaces.end()) {
    onFaceCreated(i->second);
    return;
  }

  shared_ptr<ip::tcp::socket> clientSocket =
    make_shared<ip::tcp::socket>(ref(getGlobalIoService()));

  EventId connectTimeoutEvent = scheduler::schedule(timeout,
                                                    bind(&TcpChannel::handleFailedConnect, this,
                                                         clientSocket, onConnectFailed));

  clientSocket->async_connect(remoteEndpoint,
                              bind(&TcpChannel::handleSuccessfulConnect, this, _1,
                                   clientSocket, connectTimeoutEvent,
                                   onFaceCreated, onConnectFailed));
}

void
TcpChannel::connect(const std::string& remoteHost, const std::string& remotePort,
                    const TcpChannel::FaceCreatedCallback& onFaceCreated,
                    const TcpChannel::ConnectFailedCallback& onConnectFailed,
                    const time::seconds& timeout/* = time::seconds(4)*/)
{
  shared_ptr<ip::tcp::socket> clientSocket =
    make_shared<ip::tcp::socket>(ref(getGlobalIoService()));

  ip::tcp::resolver::query query(remoteHost, remotePort);
  shared_ptr<ip::tcp::resolver> resolver =
    make_shared<ip::tcp::resolver>(ref(getGlobalIoService()));

  EventId connectTimeoutEvent = scheduler::schedule(timeout,
                                                    bind(&TcpChannel::handleFailedConnect, this,
                                                         clientSocket, onConnectFailed));

  resolver->async_resolve(query,
                          bind(&TcpChannel::handleEndpointResolution, this, _1, _2,
                               clientSocket, connectTimeoutEvent,
                               onFaceCreated, onConnectFailed,
                               resolver));
}

size_t
TcpChannel::size() const
{
  return m_channelFaces.size();
}

void
TcpChannel::createFace(const shared_ptr<ip::tcp::socket>& socket,
                       const FaceCreatedCallback& onFaceCreated,
                       bool isOnDemand)
{
  tcp::Endpoint remoteEndpoint = socket->remote_endpoint();

  shared_ptr<Face> face;

  ChannelFaceMap::iterator faceMapPos = m_channelFaces.find(remoteEndpoint);
  if (faceMapPos == m_channelFaces.end())
    {
      if (socket->local_endpoint().address().is_loopback())
        face = make_shared<TcpLocalFace>(socket, isOnDemand);
      else
        face = make_shared<TcpFace>(socket, isOnDemand);

      face->onFail += bind(&TcpChannel::afterFaceFailed, this, remoteEndpoint);

      m_channelFaces[remoteEndpoint] = face;
    }
  else
    {
      // we've already created a a face for this endpoint, just reuse it
      face = faceMapPos->second;

      boost::system::error_code error;
      socket->shutdown(ip::tcp::socket::shutdown_both, error);
      socket->close(error);
    }

  // Need to invoke the callback regardless of whether or not we have already created
  // the face so that control responses and such can be sent.
  onFaceCreated(face);
}

void
TcpChannel::afterFaceFailed(tcp::Endpoint &remoteEndpoint)
{
  NFD_LOG_DEBUG("afterFaceFailed: " << remoteEndpoint);
  m_channelFaces.erase(remoteEndpoint);
}

void
TcpChannel::handleSuccessfulAccept(const boost::system::error_code& error,
                                   const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                                   const FaceCreatedCallback& onFaceCreated,
                                   const ConnectFailedCallback& onAcceptFailed)
{
  if (error) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    NFD_LOG_DEBUG("Connect to remote endpoint failed: "
                  << error.category().message(error.value()));

    if (static_cast<bool>(onAcceptFailed))
      onAcceptFailed("Connect to remote endpoint failed: " +
                     error.category().message(error.value()));
    return;
  }

  // prepare accepting the next connection
  shared_ptr<ip::tcp::socket> clientSocket =
    make_shared<ip::tcp::socket>(ref(getGlobalIoService()));
  m_acceptor->async_accept(*clientSocket,
                           bind(&TcpChannel::handleSuccessfulAccept, this, _1,
                                clientSocket,
                                onFaceCreated, onAcceptFailed));

  NFD_LOG_DEBUG("[" << m_localEndpoint << "] "
                "<< Connection from " << socket->remote_endpoint());
  createFace(socket, onFaceCreated, true);
}

void
TcpChannel::handleSuccessfulConnect(const boost::system::error_code& error,
                                    const shared_ptr<ip::tcp::socket>& socket,
                                    const EventId& connectTimeoutEvent,
                                    const FaceCreatedCallback& onFaceCreated,
                                    const ConnectFailedCallback& onConnectFailed)
{
  scheduler::cancel(connectTimeoutEvent);

#if (BOOST_VERSION == 105400)
  // To handle regression in Boost 1.54
  // https://svn.boost.org/trac/boost/ticket/8795
  boost::system::error_code anotherErrorCode;
  socket->remote_endpoint(anotherErrorCode);
  if (error || anotherErrorCode) {
#else
  if (error) {
#endif

    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    socket->close();

    NFD_LOG_DEBUG("Connect to remote endpoint failed: "
                  << error.category().message(error.value()));

    onConnectFailed("Connect to remote endpoint failed: " +
                    error.category().message(error.value()));
    return;
  }

  NFD_LOG_DEBUG("[" << m_localEndpoint << "] "
                ">> Connection to " << socket->remote_endpoint());

  createFace(socket, onFaceCreated, false);
}

void
TcpChannel::handleFailedConnect(const shared_ptr<ip::tcp::socket>& socket,
                                const ConnectFailedCallback& onConnectFailed)
{
  NFD_LOG_DEBUG("Connect to remote endpoint timed out");
  onConnectFailed("Connect to remote endpoint timed out");
  socket->close(); // abort the connection
}

void
TcpChannel::handleEndpointResolution(const boost::system::error_code& error,
                                     ip::tcp::resolver::iterator remoteEndpoint,
                                     const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                                     const EventId& connectTimeoutEvent,
                                     const FaceCreatedCallback& onFaceCreated,
                                     const ConnectFailedCallback& onConnectFailed,
                                     const shared_ptr<ip::tcp::resolver>& resolver)
{
  if (error ||
      remoteEndpoint == ip::tcp::resolver::iterator())
    {
      if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
        return;

      socket->close();
      scheduler::cancel(connectTimeoutEvent);

      NFD_LOG_DEBUG("Remote endpoint hostname or port cannot be resolved: "
                    << error.category().message(error.value()));

      onConnectFailed("Remote endpoint hostname or port cannot be resolved: " +
                      error.category().message(error.value()));
      return;
    }

  // got endpoint, now trying to connect (only try the first resolution option)
  socket->async_connect(*remoteEndpoint,
                        bind(&TcpChannel::handleSuccessfulConnect, this, _1,
                             socket, connectTimeoutEvent,
                             onFaceCreated, onConnectFailed));
}

} // namespace nfd
