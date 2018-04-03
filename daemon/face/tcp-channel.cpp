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

#include "tcp-channel.hpp"
#include "core/global-io.hpp"
#include "generic-link-service.hpp"
#include "tcp-transport.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT("TcpChannel");

namespace ip = boost::asio::ip;

TcpChannel::TcpChannel(const tcp::Endpoint& localEndpoint, bool wantCongestionMarking,
                       DetermineFaceScopeFromAddress determineFaceScope)
  : m_localEndpoint(localEndpoint)
  , m_acceptor(getGlobalIoService())
  , m_socket(getGlobalIoService())
  , m_wantCongestionMarking(wantCongestionMarking)
  , m_determineFaceScope(std::move(determineFaceScope))
{
  setUri(FaceUri(m_localEndpoint));
  NFD_LOG_CHAN_INFO("Creating channel");
}

void
TcpChannel::listen(const FaceCreatedCallback& onFaceCreated,
                   const FaceCreationFailedCallback& onAcceptFailed,
                   int backlog/* = tcp::acceptor::max_connections*/)
{
  if (isListening()) {
    NFD_LOG_CHAN_WARN("Already listening");
    return;
  }

  m_acceptor.open(m_localEndpoint.protocol());
  m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
  if (m_localEndpoint.address().is_v6()) {
    m_acceptor.set_option(ip::v6_only(true));
  }
  m_acceptor.bind(m_localEndpoint);
  m_acceptor.listen(backlog);

  accept(onFaceCreated, onAcceptFailed);
  NFD_LOG_CHAN_DEBUG("Started listening");
}

void
TcpChannel::connect(const tcp::Endpoint& remoteEndpoint,
                    const FaceParams& params,
                    const FaceCreatedCallback& onFaceCreated,
                    const FaceCreationFailedCallback& onConnectFailed,
                    time::nanoseconds timeout)
{
  auto it = m_channelFaces.find(remoteEndpoint);
  if (it != m_channelFaces.end()) {
    NFD_LOG_CHAN_TRACE("Reusing existing face for " << remoteEndpoint);
    onFaceCreated(it->second);
    return;
  }

  auto clientSocket = make_shared<ip::tcp::socket>(ref(getGlobalIoService()));
  auto timeoutEvent = scheduler::schedule(timeout, bind(&TcpChannel::handleConnectTimeout, this,
                                                        remoteEndpoint, clientSocket, onConnectFailed));

  NFD_LOG_CHAN_TRACE("Connecting to " << remoteEndpoint);
  clientSocket->async_connect(remoteEndpoint,
                              bind(&TcpChannel::handleConnect, this,
                                   boost::asio::placeholders::error, remoteEndpoint, clientSocket,
                                   params, timeoutEvent, onFaceCreated, onConnectFailed));
}

void
TcpChannel::createFace(ip::tcp::socket&& socket,
                       const FaceParams& params,
                       const FaceCreatedCallback& onFaceCreated)
{
  shared_ptr<Face> face;
  tcp::Endpoint remoteEndpoint = socket.remote_endpoint();

  auto it = m_channelFaces.find(remoteEndpoint);
  if (it == m_channelFaces.end()) {
    GenericLinkService::Options options;
    options.allowLocalFields = params.wantLocalFields;
    options.reliabilityOptions.isEnabled = params.wantLpReliability;

    if (boost::logic::indeterminate(params.wantCongestionMarking)) {
      // Use default value for this channel if parameter is indeterminate
      options.allowCongestionMarking = m_wantCongestionMarking;
    }
    else {
      options.allowCongestionMarking = params.wantCongestionMarking;
    }

    if (params.baseCongestionMarkingInterval) {
      options.baseCongestionMarkingInterval = *params.baseCongestionMarkingInterval;
    }
    if (params.defaultCongestionThreshold) {
      options.defaultCongestionThreshold = *params.defaultCongestionThreshold;
    }

    auto linkService = make_unique<GenericLinkService>(options);
    auto faceScope = m_determineFaceScope(socket.local_endpoint().address(),
                                          socket.remote_endpoint().address());
    auto transport = make_unique<TcpTransport>(std::move(socket), params.persistency, faceScope);
    face = make_shared<Face>(std::move(linkService), std::move(transport));

    m_channelFaces[remoteEndpoint] = face;
    connectFaceClosedSignal(*face, [this, remoteEndpoint] { m_channelFaces.erase(remoteEndpoint); });
  }
  else {
    // we already have a face for this endpoint, just reuse it
    face = it->second;
    NFD_LOG_CHAN_TRACE("Reusing existing face for " << remoteEndpoint);

    boost::system::error_code error;
    socket.shutdown(ip::tcp::socket::shutdown_both, error);
    socket.close(error);
  }

  // Need to invoke the callback regardless of whether or not we have already created
  // the face so that control responses and such can be sent.
  onFaceCreated(face);
}

void
TcpChannel::accept(const FaceCreatedCallback& onFaceCreated,
                   const FaceCreationFailedCallback& onAcceptFailed)
{
  m_acceptor.async_accept(m_socket, bind(&TcpChannel::handleAccept, this,
                                         boost::asio::placeholders::error,
                                         onFaceCreated, onAcceptFailed));
}

void
TcpChannel::handleAccept(const boost::system::error_code& error,
                         const FaceCreatedCallback& onFaceCreated,
                         const FaceCreationFailedCallback& onAcceptFailed)
{
  if (error) {
    if (error != boost::asio::error::operation_aborted) {
      NFD_LOG_CHAN_DEBUG("Accept failed: " << error.message());
      if (onAcceptFailed)
        onAcceptFailed(500, "Accept failed: " + error.message());
    }
    return;
  }

  NFD_LOG_CHAN_TRACE("Incoming connection from " << m_socket.remote_endpoint());

  FaceParams params;
  params.persistency = ndn::nfd::FACE_PERSISTENCY_ON_DEMAND;
  createFace(std::move(m_socket), params, onFaceCreated);

  // prepare accepting the next connection
  accept(onFaceCreated, onAcceptFailed);
}

void
TcpChannel::handleConnect(const boost::system::error_code& error,
                          const tcp::Endpoint& remoteEndpoint,
                          const shared_ptr<ip::tcp::socket>& socket,
                          const FaceParams& params,
                          const scheduler::EventId& connectTimeoutEvent,
                          const FaceCreatedCallback& onFaceCreated,
                          const FaceCreationFailedCallback& onConnectFailed)
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
    if (error != boost::asio::error::operation_aborted) {
      NFD_LOG_CHAN_DEBUG("Connection to " << remoteEndpoint << " failed: " << error.message());
      if (onConnectFailed)
        onConnectFailed(504, "Connection failed: " + error.message());
    }
    return;
  }

  NFD_LOG_CHAN_TRACE("Connected to " << socket->remote_endpoint());
  createFace(std::move(*socket), params, onFaceCreated);
}

void
TcpChannel::handleConnectTimeout(const tcp::Endpoint& remoteEndpoint,
                                 const shared_ptr<ip::tcp::socket>& socket,
                                 const FaceCreationFailedCallback& onConnectFailed)
{
  NFD_LOG_CHAN_DEBUG("Connection to " << remoteEndpoint << " timed out");

  // abort the connection attempt
  boost::system::error_code error;
  socket->close(error);

  if (onConnectFailed)
    onConnectFailed(504, "Connection timed out");
}

} // namespace face
} // namespace nfd
