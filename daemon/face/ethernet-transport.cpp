/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "ethernet-transport.hpp"
#include "ethernet-protocol.hpp"
#include "core/global-io.hpp"

#include <pcap/pcap.h>

#include <arpa/inet.h>  // for htons()
#include <cstring>      // for memcpy()

namespace nfd {
namespace face {

NFD_LOG_INIT("EthernetTransport");

EthernetTransport::EthernetTransport(const ndn::net::NetworkInterface& localEndpoint,
                                     const ethernet::Address& remoteEndpoint)
  : m_socket(getGlobalIoService())
  , m_pcap(localEndpoint.getName())
  , m_srcAddress(localEndpoint.getEthernetAddress())
  , m_destAddress(remoteEndpoint)
  , m_interfaceName(localEndpoint.getName())
  , m_hasRecentlyReceived(false)
#ifdef _DEBUG
  , m_nDropped(0)
#endif
{
  try {
    m_pcap.activate(DLT_EN10MB);
    m_socket.assign(m_pcap.getFd());
  }
  catch (const PcapHelper::Error& e) {
    BOOST_THROW_EXCEPTION(Error(e.what()));
  }

  asyncRead();
}

void
EthernetTransport::doClose()
{
  NFD_LOG_FACE_TRACE(__func__);

  if (m_socket.is_open()) {
    // Cancel all outstanding operations and close the socket.
    // Use the non-throwing variants and ignore errors, if any.
    boost::system::error_code error;
    m_socket.cancel(error);
    m_socket.close(error);
  }
  m_pcap.close();

  // Ensure that the Transport stays alive at least
  // until all pending handlers are dispatched
  getGlobalIoService().post([this] {
    this->setState(TransportState::CLOSED);
  });
}

void
EthernetTransport::doSend(Transport::Packet&& packet)
{
  NFD_LOG_FACE_TRACE(__func__);

  sendPacket(packet.packet);
}

void
EthernetTransport::sendPacket(const ndn::Block& block)
{
  ndn::EncodingBuffer buffer(block);

  // pad with zeroes if the payload is too short
  if (block.size() < ethernet::MIN_DATA_LEN) {
    static const uint8_t padding[ethernet::MIN_DATA_LEN] = {};
    buffer.appendByteArray(padding, ethernet::MIN_DATA_LEN - block.size());
  }

  // construct and prepend the ethernet header
  static uint16_t ethertype = htons(ethernet::ETHERTYPE_NDN);
  buffer.prependByteArray(reinterpret_cast<const uint8_t*>(&ethertype), ethernet::TYPE_LEN);
  buffer.prependByteArray(m_srcAddress.data(), m_srcAddress.size());
  buffer.prependByteArray(m_destAddress.data(), m_destAddress.size());

  // send the frame
  int sent = pcap_inject(m_pcap, buffer.buf(), buffer.size());
  if (sent < 0)
    handleError("Send operation failed: " + m_pcap.getLastError());
  else if (static_cast<size_t>(sent) < buffer.size())
    handleError("Failed to send the full frame: size=" + to_string(buffer.size()) +
                " sent=" + to_string(sent));
  else
    // print block size because we don't want to count the padding in buffer
    NFD_LOG_FACE_TRACE("Successfully sent: " << block.size() << " bytes");
}

void
EthernetTransport::asyncRead()
{
  m_socket.async_read_some(boost::asio::null_buffers(),
                           bind(&EthernetTransport::handleRead, this,
                                boost::asio::placeholders::error));
}

void
EthernetTransport::handleRead(const boost::system::error_code& error)
{
  if (error) {
    // boost::asio::error::operation_aborted must be checked first: in that case, the Transport
    // may already have been destructed, therefore it's unsafe to call getState() or do logging.
    if (error != boost::asio::error::operation_aborted &&
        getState() != TransportState::CLOSING &&
        getState() != TransportState::FAILED &&
        getState() != TransportState::CLOSED) {
      handleError("Receive operation failed: " + error.message());
    }
    return;
  }

  const uint8_t* pkt;
  size_t len;
  std::string err;
  std::tie(pkt, len, err) = m_pcap.readNextPacket();

  if (pkt == nullptr) {
    NFD_LOG_FACE_WARN("Read error: " << err);
  }
  else {
    const ether_header* eh;
    std::tie(eh, err) = ethernet::checkFrameHeader(pkt, len, m_srcAddress,
                                                   m_destAddress.isMulticast() ? m_destAddress : m_srcAddress);
    if (eh == nullptr) {
      NFD_LOG_FACE_WARN(err);
    }
    else {
      ethernet::Address sender(eh->ether_shost);
      pkt += ethernet::HDR_LEN;
      len -= ethernet::HDR_LEN;
      receivePayload(pkt, len, sender);
    }
  }

#ifdef _DEBUG
  size_t nDropped = m_pcap.getNDropped();
  if (nDropped - m_nDropped > 0)
    NFD_LOG_FACE_DEBUG("Detected " << nDropped - m_nDropped << " dropped frame(s)");
  m_nDropped = nDropped;
#endif

  asyncRead();
}

void
EthernetTransport::receivePayload(const uint8_t* payload, size_t length,
                                  const ethernet::Address& sender)
{
  NFD_LOG_FACE_TRACE("Received: " << length << " bytes from " << sender);

  bool isOk = false;
  Block element;
  std::tie(isOk, element) = Block::fromBuffer(payload, length);
  if (!isOk) {
    NFD_LOG_FACE_WARN("Failed to parse incoming packet from " << sender);
    // This packet won't extend the face lifetime
    return;
  }
  m_hasRecentlyReceived = true;

  Transport::Packet tp(std::move(element));
  static_assert(sizeof(tp.remoteEndpoint) >= ethernet::ADDR_LEN,
                "Transport::Packet::remoteEndpoint is too small");
  if (m_destAddress.isMulticast()) {
    std::memcpy(&tp.remoteEndpoint, sender.data(), sender.size());
  }
  this->receive(std::move(tp));
}

void
EthernetTransport::handleError(const std::string& errorMessage)
{
  if (getPersistency() == ndn::nfd::FACE_PERSISTENCY_PERMANENT) {
    NFD_LOG_FACE_DEBUG("Permanent face ignores error: " << errorMessage);
    return;
  }

  NFD_LOG_FACE_ERROR(errorMessage);
  this->setState(TransportState::FAILED);
  doClose();
}

} // namespace face
} // namespace nfd
