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

#include "ethernet-channel.hpp"
#include "ethernet-protocol.hpp"
#include "generic-link-service.hpp"
#include "unicast-ethernet-transport.hpp"
#include "core/global-io.hpp"

#include <boost/range/adaptor/map.hpp>
#include <pcap/pcap.h>

namespace nfd {
namespace face {

NFD_LOG_INIT("EthernetChannel");

EthernetChannel::EthernetChannel(shared_ptr<const ndn::net::NetworkInterface> localEndpoint,
                                 time::nanoseconds idleTimeout)
  : m_localEndpoint(std::move(localEndpoint))
  , m_isListening(false)
  , m_socket(getGlobalIoService())
  , m_pcap(m_localEndpoint->getName())
  , m_idleFaceTimeout(idleTimeout)
#ifdef _DEBUG
  , m_nDropped(0)
#endif
{
  setUri(FaceUri::fromDev(m_localEndpoint->getName()));
  NFD_LOG_CHAN_INFO("Creating channel");
}

void
EthernetChannel::connect(const ethernet::Address& remoteEndpoint,
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
EthernetChannel::listen(const FaceCreatedCallback& onFaceCreated,
                        const FaceCreationFailedCallback& onFaceCreationFailed)
{
  if (isListening()) {
    NFD_LOG_CHAN_WARN("Already listening");
    return;
  }
  m_isListening = true;

  try {
    m_pcap.activate(DLT_EN10MB);
    m_socket.assign(m_pcap.getFd());
  }
  catch (const PcapHelper::Error& e) {
    BOOST_THROW_EXCEPTION(Error(e.what()));
  }
  updateFilter();

  asyncRead(onFaceCreated, onFaceCreationFailed);
  NFD_LOG_CHAN_DEBUG("Started listening");
}

void
EthernetChannel::asyncRead(const FaceCreatedCallback& onFaceCreated,
                           const FaceCreationFailedCallback& onReceiveFailed)
{
  m_socket.async_read_some(boost::asio::null_buffers(),
                           bind(&EthernetChannel::handleRead, this,
                                boost::asio::placeholders::error,
                                onFaceCreated, onReceiveFailed));
}

void
EthernetChannel::handleRead(const boost::system::error_code& error,
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

  const uint8_t* pkt;
  size_t len;
  std::string err;
  std::tie(pkt, len, err) = m_pcap.readNextPacket();

  if (pkt == nullptr) {
    NFD_LOG_CHAN_WARN("Read error: " << err);
  }
  else {
    const ether_header* eh;
    std::tie(eh, err) = ethernet::checkFrameHeader(pkt, len, m_localEndpoint->getEthernetAddress(),
                                                   m_localEndpoint->getEthernetAddress());
    if (eh == nullptr) {
      NFD_LOG_CHAN_DEBUG(err);
    }
    else {
      ethernet::Address sender(eh->ether_shost);
      pkt += ethernet::HDR_LEN;
      len -= ethernet::HDR_LEN;
      processIncomingPacket(pkt, len, sender, onFaceCreated, onReceiveFailed);
    }
  }

#ifdef _DEBUG
  size_t nDropped = m_pcap.getNDropped();
  if (nDropped - m_nDropped > 0)
    NFD_LOG_CHAN_DEBUG("Detected " << nDropped - m_nDropped << " dropped frame(s)");
  m_nDropped = nDropped;
#endif

  asyncRead(onFaceCreated, onReceiveFailed);
}

void
EthernetChannel::processIncomingPacket(const uint8_t* packet, size_t length,
                                       const ethernet::Address& sender,
                                       const FaceCreatedCallback& onFaceCreated,
                                       const FaceCreationFailedCallback& onReceiveFailed)
{
  NFD_LOG_CHAN_TRACE("New peer " << sender);

  bool isCreated = false;
  shared_ptr<Face> face;
  try {
    FaceParams params;
    params.persistency = ndn::nfd::FACE_PERSISTENCY_ON_DEMAND;
    std::tie(isCreated, face) = createFace(sender, params);
  }
  catch (const EthernetTransport::Error& e) {
    NFD_LOG_CHAN_DEBUG("Face creation for " << sender << " failed: " << e.what());
    if (onReceiveFailed)
      onReceiveFailed(504, std::string("Face creation failed: ") + e.what());
    return;
  }

  if (isCreated)
    onFaceCreated(face);
  else
    NFD_LOG_CHAN_DEBUG("Received frame for existing face");

  // dispatch the packet to the face for processing
  auto* transport = static_cast<UnicastEthernetTransport*>(face->getTransport());
  transport->receivePayload(packet, length, sender);
}

std::pair<bool, shared_ptr<Face>>
EthernetChannel::createFace(const ethernet::Address& remoteEndpoint,
                            const FaceParams& params)
{
  auto it = m_channelFaces.find(remoteEndpoint);
  if (it != m_channelFaces.end()) {
    // we already have a face for this endpoint, so reuse it
    NFD_LOG_CHAN_TRACE("Reusing existing face for " << remoteEndpoint);
    return {false, it->second};
  }

  // else, create a new face
  GenericLinkService::Options options;
  options.allowFragmentation = true;
  options.allowReassembly = true;
  options.reliabilityOptions.isEnabled = params.wantLpReliability;

  auto linkService = make_unique<GenericLinkService>(options);
  auto transport = make_unique<UnicastEthernetTransport>(*m_localEndpoint, remoteEndpoint,
                                                         params.persistency, m_idleFaceTimeout);
  auto face = make_shared<Face>(std::move(linkService), std::move(transport));

  m_channelFaces[remoteEndpoint] = face;
  connectFaceClosedSignal(*face, [this, remoteEndpoint] {
    m_channelFaces.erase(remoteEndpoint);
    updateFilter();
  });
  updateFilter();

  return {true, face};
}

void
EthernetChannel::updateFilter()
{
  if (!isListening())
    return;

  std::string filter = "(ether proto " + to_string(ethernet::ETHERTYPE_NDN) +
                       ") && (ether dst " + m_localEndpoint->getEthernetAddress().toString() + ")";
  for (const auto& addr : m_channelFaces | boost::adaptors::map_keys) {
    filter += " && (not ether src " + addr.toString() + ")";
  }
  // "not vlan" must appear last in the filter expression, or the
  // rest of the filter won't work as intended, see pcap-filter(7)
  filter += " && (not vlan)";

  NFD_LOG_CHAN_TRACE("Updating filter: " << filter);
  m_pcap.setPacketFilter(filter.data());
}

} // namespace face
} // namespace nfd
