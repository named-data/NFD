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

#include "udp-channel.hpp"
#include "generic-link-service.hpp"
#include "unicast-udp-transport.hpp"
#include "core/global-io.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT("UdpChannel");

namespace ip = boost::asio::ip;

UdpChannel::UdpChannel(const udp::Endpoint& localEndpoint,
                       time::nanoseconds idleTimeout,
                       bool wantCongestionMarking)
  : m_localEndpoint(localEndpoint)
  , m_socket(getGlobalIoService())
  , m_idleFaceTimeout(idleTimeout)
  , m_wantCongestionMarking(wantCongestionMarking)
{
  setUri(FaceUri(m_localEndpoint));
  NFD_LOG_CHAN_INFO("Creating channel");
}

void
UdpChannel::connect(const udp::Endpoint& remoteEndpoint,
                    const FaceParams& params,
                    const FaceCreatedCallback& onFaceCreated,
                    const FaceCreationFailedCallback& onConnectFailed)
{
  shared_ptr<Face> face;
  try {
    face = createFace(remoteEndpoint, params).second;
  }
  catch (const boost::system::system_error& e) {
    NFD_LOG_CHAN_DEBUG("Face creation for " << remoteEndpoint << " failed: " << e.what());
    if (onConnectFailed)
      onConnectFailed(504, std::string("Face creation failed: ") + e.what());
    return;
  }

  // Need to invoke the callback regardless of whether or not we had already
  // created the face so that control responses and such can be sent
  onFaceCreated(face);
}

void
UdpChannel::listen(const FaceCreatedCallback& onFaceCreated,
                   const FaceCreationFailedCallback& onFaceCreationFailed)
{
  if (isListening()) {
    NFD_LOG_CHAN_WARN("Already listening");
    return;
  }

  m_socket.open(m_localEndpoint.protocol());
  m_socket.set_option(ip::udp::socket::reuse_address(true));
  if (m_localEndpoint.address().is_v6()) {
    m_socket.set_option(ip::v6_only(true));
  }
  m_socket.bind(m_localEndpoint);

  waitForNewPeer(onFaceCreated, onFaceCreationFailed);
  NFD_LOG_CHAN_DEBUG("Started listening");
}

void
UdpChannel::waitForNewPeer(const FaceCreatedCallback& onFaceCreated,
                           const FaceCreationFailedCallback& onReceiveFailed)
{
  m_socket.async_receive_from(boost::asio::buffer(m_receiveBuffer), m_remoteEndpoint,
                              bind(&UdpChannel::handleNewPeer, this,
                                   boost::asio::placeholders::error,
                                   boost::asio::placeholders::bytes_transferred,
                                   onFaceCreated, onReceiveFailed));
}

void
UdpChannel::handleNewPeer(const boost::system::error_code& error,
                          size_t nBytesReceived,
                          const FaceCreatedCallback& onFaceCreated,
                          const FaceCreationFailedCallback& onReceiveFailed)
{
  if (error) {
    if (error != boost::asio::error::operation_aborted) {
      NFD_LOG_CHAN_DEBUG("Receive failed: " << error.message());
      if (onReceiveFailed)
        onReceiveFailed(500, "Receive failed: " + error.message());
    }
    return;
  }

  NFD_LOG_CHAN_TRACE("New peer " << m_remoteEndpoint);

  bool isCreated = false;
  shared_ptr<Face> face;
  try {
    FaceParams params;
    params.persistency = ndn::nfd::FACE_PERSISTENCY_ON_DEMAND;
    std::tie(isCreated, face) = createFace(m_remoteEndpoint, params);
  }
  catch (const boost::system::system_error& e) {
    NFD_LOG_CHAN_DEBUG("Face creation for " << m_remoteEndpoint << " failed: " << e.what());
    if (onReceiveFailed)
      onReceiveFailed(504, std::string("Face creation failed: ") + e.what());
    return;
  }

  if (isCreated)
    onFaceCreated(face);
  else
    NFD_LOG_CHAN_DEBUG("Received datagram for existing face");

  // dispatch the datagram to the face for processing
  auto* transport = static_cast<UnicastUdpTransport*>(face->getTransport());
  transport->receiveDatagram(m_receiveBuffer.data(), nBytesReceived, error);

  waitForNewPeer(onFaceCreated, onReceiveFailed);
}

std::pair<bool, shared_ptr<Face>>
UdpChannel::createFace(const udp::Endpoint& remoteEndpoint,
                       const FaceParams& params)
{
  auto it = m_channelFaces.find(remoteEndpoint);
  if (it != m_channelFaces.end()) {
    // we already have a face for this endpoint, so reuse it
    NFD_LOG_CHAN_TRACE("Reusing existing face for " << remoteEndpoint);
    return {false, it->second};
  }

  // else, create a new face
  ip::udp::socket socket(getGlobalIoService(), m_localEndpoint.protocol());
  socket.set_option(ip::udp::socket::reuse_address(true));
  socket.bind(m_localEndpoint);
  socket.connect(remoteEndpoint);

  GenericLinkService::Options options;
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
  auto transport = make_unique<UnicastUdpTransport>(std::move(socket), params.persistency, m_idleFaceTimeout);
  auto face = make_shared<Face>(std::move(linkService), std::move(transport));

  m_channelFaces[remoteEndpoint] = face;
  connectFaceClosedSignal(*face, [this, remoteEndpoint] { m_channelFaces.erase(remoteEndpoint); });

  return {true, face};
}

} // namespace face
} // namespace nfd
