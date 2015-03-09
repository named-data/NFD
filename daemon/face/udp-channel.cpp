/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "udp-channel.hpp"
#include "udp-face.hpp"
#include "core/global-io.hpp"

namespace nfd {

NFD_LOG_INIT("UdpChannel");

using namespace boost::asio;

UdpChannel::UdpChannel(const udp::Endpoint& localEndpoint,
                       const time::seconds& timeout)
  : m_localEndpoint(localEndpoint)
  , m_isListening(false)
  , m_idleFaceTimeout(timeout)
{
  setUri(FaceUri(m_localEndpoint));

  m_socket = make_shared<ip::udp::socket>(ref(getGlobalIoService()));
  m_socket->open(m_localEndpoint.protocol());
  m_socket->set_option(ip::udp::socket::reuse_address(true));
  if (m_localEndpoint.address().is_v6())
    m_socket->set_option(ip::v6_only(true));

  try {
    m_socket->bind(m_localEndpoint);
  }
  catch (const boost::system::system_error& e) {
    // bind failed, so the socket is useless now
    m_socket->close();
    throw Error("bind failed: " + std::string(e.what()));
  }
}

void
UdpChannel::listen(const FaceCreatedCallback& onFaceCreated,
                   const ConnectFailedCallback& onReceiveFailed)
{
  if (isListening()) {
    NFD_LOG_WARN("[" << m_localEndpoint << "] Already listening");
    return;
  }
  m_isListening = true;

  m_socket->async_receive_from(boost::asio::buffer(m_inputBuffer, ndn::MAX_NDN_PACKET_SIZE),
                               m_newRemoteEndpoint,
                               bind(&UdpChannel::handleNewPeer, this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred,
                                    onFaceCreated, onReceiveFailed));
}

void
UdpChannel::connect(const udp::Endpoint& remoteEndpoint,
                    const FaceCreatedCallback& onFaceCreated,
                    const ConnectFailedCallback& onConnectFailed)
{
  auto it = m_channelFaces.find(remoteEndpoint);
  if (it != m_channelFaces.end()) {
    it->second->setOnDemand(false);
    onFaceCreated(it->second);
    return;
  }

  // creating a new socket for the face that will be created soon
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
  catch (const boost::system::system_error& e) {
    clientSocket->close();
    onConnectFailed("Failed to configure socket (" + std::string(e.what()) + ")");
    return;
  }

  createFace(clientSocket, onFaceCreated, false);
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

  shared_ptr<UdpFace> face;

  auto it = m_channelFaces.find(remoteEndpoint);
  if (it == m_channelFaces.end())
    {
      face = make_shared<UdpFace>(socket, isOnDemand, m_idleFaceTimeout);

      face->onFail.connectSingleShot([this, remoteEndpoint] (const std::string&) {
        NFD_LOG_TRACE("Erasing " << remoteEndpoint << " from channel face map");
        m_channelFaces.erase(remoteEndpoint);
      });

      m_channelFaces[remoteEndpoint] = face;
    }
  else
    {
      // we've already created a a face for this endpoint, just reuse it
      face = it->second;

      boost::system::error_code error;
      socket->shutdown(ip::udp::socket::shutdown_both, error);
      socket->close(error);
    }

  // Need to invoke the callback regardless of whether or not we have already created
  // the face so that control responses and such can be sent.
  onFaceCreated(face);

  return face;
}

void
UdpChannel::handleNewPeer(const boost::system::error_code& error,
                          size_t nBytesReceived,
                          const FaceCreatedCallback& onFaceCreated,
                          const ConnectFailedCallback& onReceiveFailed)
{
  if (error) {
    if (error == boost::asio::error::operation_aborted) // when the socket is closed by someone
      return;

    NFD_LOG_DEBUG("[" << m_localEndpoint << "] Receive failed: " << error.message());
    if (onReceiveFailed)
      onReceiveFailed(error.message());
    return;
  }

  NFD_LOG_DEBUG("[" << m_localEndpoint << "] New peer " << m_newRemoteEndpoint);

  shared_ptr<UdpFace> face;

  auto it = m_channelFaces.find(m_newRemoteEndpoint);
  if (it != m_channelFaces.end()) {
    //The face already exists.
    //Usually this shouldn't happen, because the channel creates a Udpface
    //as soon as it receives a pkt from a new endpoint and then the
    //traffic is dispatched by the kernel directly to the face.
    //However, if the node receives multiple packets from the same endpoint
    //"at the same time", while the channel is creating the face the kernel
    //could dispatch the other pkts to the channel because the face is not yet
    //ready. In this case, the channel has to pass the pkt to the face

    NFD_LOG_DEBUG("The creation of the face for the remote endpoint "
                  << m_newRemoteEndpoint << " is already in progress");
    face = it->second;
  }
  else {
    shared_ptr<ip::udp::socket> clientSocket =
      make_shared<ip::udp::socket>(ref(getGlobalIoService()));
    clientSocket->open(m_localEndpoint.protocol());
    clientSocket->set_option(ip::udp::socket::reuse_address(true));
    clientSocket->bind(m_localEndpoint);

    boost::system::error_code ec;
    clientSocket->connect(m_newRemoteEndpoint, ec);
    if (ec) {
      NFD_LOG_WARN("Error while creating on-demand UDP face from " << m_newRemoteEndpoint << ": "
                   << boost::system::system_error(ec).what());
      return;
    }

    face = createFace(clientSocket, onFaceCreated, true);
  }

  // dispatch the datagram to the face for processing
  face->receiveDatagram(m_inputBuffer, nBytesReceived, error);

  m_socket->async_receive_from(boost::asio::buffer(m_inputBuffer, ndn::MAX_NDN_PACKET_SIZE),
                               m_newRemoteEndpoint,
                               bind(&UdpChannel::handleNewPeer, this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred,
                                    onFaceCreated, onReceiveFailed));
}

} // namespace nfd
