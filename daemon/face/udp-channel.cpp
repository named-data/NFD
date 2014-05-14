/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "udp-channel.hpp"
#include "core/global-io.hpp"
#include "core/face-uri.hpp"

namespace nfd {

NFD_LOG_INIT("UdpChannel");

using namespace boost::asio;

UdpChannel::UdpChannel(const udp::Endpoint& localEndpoint,
                       const time::seconds& timeout)
  : m_localEndpoint(localEndpoint)
  , m_isListening(false)
  , m_idleFaceTimeout(timeout)
{
  /// \todo the reuse_address works as we want in Linux, but in other system could be different.
  ///       We need to check this
  ///       (SO_REUSEADDR doesn't behave uniformly in different OS)

  m_socket = make_shared<ip::udp::socket>(ref(getGlobalIoService()));
  m_socket->open(m_localEndpoint.protocol());
  m_socket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
  if (m_localEndpoint.address().is_v6())
    {
      m_socket->set_option(ip::v6_only(true));
    }

  try {
    m_socket->bind(m_localEndpoint);
  }
  catch (boost::system::system_error& e) {
    //The bind failed, so the socket is useless now
    m_socket->close();
    throw Error("Failed to properly configure the socket. "
                "UdpChannel creation aborted, check the address (" + std::string(e.what()) + ")");
  }

  this->setUri(FaceUri(localEndpoint));

  //setting the timeout to close the idle faces
  m_closeIdleFaceEvent = scheduler::schedule(m_idleFaceTimeout,
                                bind(&UdpChannel::closeIdleFaces, this));
}

UdpChannel::~UdpChannel()
{
  scheduler::cancel(m_closeIdleFaceEvent);
}

void
UdpChannel::listen(const FaceCreatedCallback& onFaceCreated,
                   const ConnectFailedCallback& onListenFailed)
{
  if (m_isListening) {
    throw Error("Listen already called on this channel");
  }
  m_isListening = true;

  onFaceCreatedNewPeerCallback = onFaceCreated;
  onConnectFailedNewPeerCallback = onListenFailed;

  m_socket->async_receive_from(boost::asio::buffer(m_inputBuffer, MAX_NDN_PACKET_SIZE),
                               m_newRemoteEndpoint,
                               bind(&UdpChannel::newPeer, this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
}


void
UdpChannel::connect(const udp::Endpoint& remoteEndpoint,
                    const FaceCreatedCallback& onFaceCreated,
                    const ConnectFailedCallback& onConnectFailed)
{
  ChannelFaceMap::iterator i = m_channelFaces.find(remoteEndpoint);
  if (i != m_channelFaces.end()) {
    i->second->setOnDemand(false);
    onFaceCreated(i->second);
    return;
  }

  //creating a new socket for the face that will be created soon
  shared_ptr<ip::udp::socket> clientSocket =
    make_shared<ip::udp::socket>(ref(getGlobalIoService()));

  clientSocket->open(m_localEndpoint.protocol());
  clientSocket->set_option(ip::udp::socket::reuse_address(true));

  try {
    clientSocket->bind(m_localEndpoint);
    clientSocket->connect(remoteEndpoint); //@todo connect or async_connect
    //(since there is no handshake the connect shouldn't block). If we go for
    //async_connect, make sure that if in the meantime we receive a UDP pkt from
    //that endpoint nothing bad happen (it's difficult, but it could happen)
  }
  catch (boost::system::system_error& e) {
    clientSocket->close();
    onConnectFailed("Failed to configure socket (" + std::string(e.what()) + ")");
    return;
  }
  createFace(clientSocket, onFaceCreated, false);
}

void
UdpChannel::connect(const std::string& remoteHost,
                    const std::string& remotePort,
                    const FaceCreatedCallback& onFaceCreated,
                    const ConnectFailedCallback& onConnectFailed)
{
  ip::udp::resolver::query query(remoteHost, remotePort);
  shared_ptr<ip::udp::resolver> resolver =
    make_shared<ip::udp::resolver>(ref(getGlobalIoService()));

  resolver->async_resolve(query,
                          bind(&UdpChannel::handleEndpointResolution, this, _1, _2,
                               onFaceCreated, onConnectFailed,
                               resolver));
}

void
UdpChannel::handleEndpointResolution(const boost::system::error_code& error,
                                      ip::udp::resolver::iterator remoteEndpoint,
                                      const FaceCreatedCallback& onFaceCreated,
                                      const ConnectFailedCallback& onConnectFailed,
                                      const shared_ptr<ip::udp::resolver>& resolver)
{
  if (error != 0 ||
      remoteEndpoint == ip::udp::resolver::iterator())
  {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    NFD_LOG_DEBUG("Remote endpoint hostname or port cannot be resolved: "
                    << error.category().message(error.value()));

    onConnectFailed("Remote endpoint hostname or port cannot be resolved: " +
                      error.category().message(error.value()));
      return;
  }

  connect(*remoteEndpoint, onFaceCreated, onConnectFailed);
}

size_t
UdpChannel::size() const
{
  return m_channelFaces.size();
}


shared_ptr<UdpFace>
UdpChannel::createFace(const shared_ptr<ip::udp::socket>& socket,
                       const FaceCreatedCallback& onFaceCreated,
                       bool isOnDemand)
{
  udp::Endpoint remoteEndpoint = socket->remote_endpoint();

  shared_ptr<UdpFace> face = make_shared<UdpFace>(socket, isOnDemand);
  face->onFail += bind(&UdpChannel::afterFaceFailed, this, remoteEndpoint);

  onFaceCreated(face);
  m_channelFaces[remoteEndpoint] = face;
  return face;
}

void
UdpChannel::newPeer(const boost::system::error_code& error,
                    std::size_t nBytesReceived)
{
  NFD_LOG_DEBUG("UdpChannel::newPeer from " << m_newRemoteEndpoint);

  shared_ptr<UdpFace> face;

  ChannelFaceMap::iterator i = m_channelFaces.find(m_newRemoteEndpoint);
  if (i != m_channelFaces.end()) {
    //The face already exists.
    //Usually this shouldn't happen, because the channel creates a Udpface
    //as soon as it receives a pkt from a new endpoint and then the
    //traffic is dispatched by the kernel directly to the face.
    //However, if the node receives multiple packets from the same endpoint
    //"at the same time", while the channel is creating the face the kernel
    //could dispatch the other pkts to the channel because the face is not yet
    //ready. In this case, the channel has to pass the pkt to the face

    NFD_LOG_DEBUG("The creation of the face for the remote endpoint "
                  << m_newRemoteEndpoint
                  << " is in progress");

    face = i->second;
  }
  else {
    shared_ptr<ip::udp::socket> clientSocket =
      make_shared<ip::udp::socket>(ref(getGlobalIoService()));
    clientSocket->open(m_localEndpoint.protocol());
    clientSocket->set_option(ip::udp::socket::reuse_address(true));
    clientSocket->bind(m_localEndpoint);
    clientSocket->connect(m_newRemoteEndpoint);

    face = createFace(clientSocket,
                      onFaceCreatedNewPeerCallback,
                      true);
  }

  //Passing the message to the correspondent face
  face->handleFirstReceive(m_inputBuffer, nBytesReceived, error);

  m_socket->async_receive_from(boost::asio::buffer(m_inputBuffer, MAX_NDN_PACKET_SIZE),
                               m_newRemoteEndpoint,
                               bind(&UdpChannel::newPeer, this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
}


void
UdpChannel::afterFaceFailed(udp::Endpoint &endpoint)
{
  NFD_LOG_DEBUG("afterFaceFailed: " << endpoint);
  m_channelFaces.erase(endpoint);
}

void
UdpChannel::closeIdleFaces()
{
  ChannelFaceMap::iterator next =  m_channelFaces.begin();

  while (next != m_channelFaces.end()) {
    ChannelFaceMap::iterator it = next;
    next++;
    if (it->second->isOnDemand() &&
        !it->second->hasBeenUsedRecently()) {
      //face has been idle since the last time closeIdleFaces
      //has been called. Going to close it
      NFD_LOG_DEBUG("Found idle face id: " << it->second->getId());
      it->second->close();
    } else {
      it->second->resetRecentUsage();
    }
  }
  m_closeIdleFaceEvent = scheduler::schedule(m_idleFaceTimeout,
                                             bind(&UdpChannel::closeIdleFaces, this));
}

} // namespace nfd
