/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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
#include "core/global-io.hpp"

#include <pcap/pcap.h>

#include <cerrno>         // for errno
#include <cstring>        // for memcpy(), strerror(), strncpy()
#include <arpa/inet.h>    // for htons() and ntohs()
#include <net/ethernet.h> // for struct ether_header
#include <net/if.h>       // for struct ifreq
#include <sys/ioctl.h>    // for ioctl()

namespace nfd {
namespace face {

NFD_LOG_INIT("EthernetTransport");

EthernetTransport::EthernetTransport(const NetworkInterfaceInfo& localEndpoint,
                                     const ethernet::Address& remoteEndpoint)
  : m_socket(getGlobalIoService())
  , m_pcap(localEndpoint.name)
  , m_srcAddress(localEndpoint.etherAddress)
  , m_destAddress(remoteEndpoint)
  , m_interfaceName(localEndpoint.name)
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

  // do this after assigning m_socket because getInterfaceMtu uses it
  this->setMtu(getInterfaceMtu());

  m_socket.async_read_some(boost::asio::null_buffers(),
                           bind(&EthernetTransport::handleRead, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
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

  // send the packet
  int sent = pcap_inject(m_pcap, buffer.buf(), buffer.size());
  if (sent < 0)
    NFD_LOG_FACE_ERROR("pcap_inject failed: " << m_pcap.getLastError());
  else if (static_cast<size_t>(sent) < buffer.size())
    NFD_LOG_FACE_ERROR("Failed to send the full frame: bufsize=" << buffer.size() << " sent=" << sent);
  else
    // print block size because we don't want to count the padding in buffer
    NFD_LOG_FACE_TRACE("Successfully sent: " << block.size() << " bytes");
}

void
EthernetTransport::handleRead(const boost::system::error_code& error, size_t)
{
  if (error)
    return processErrorCode(error);

  const uint8_t* pkt;
  size_t len;
  std::string err;
  std::tie(pkt, len, err) = m_pcap.readNextPacket();

  if (pkt == nullptr)
    NFD_LOG_FACE_ERROR("Read error: " << err);
  else
    processIncomingPacket(pkt, len);

#ifdef _DEBUG
  size_t nDropped = m_pcap.getNDropped();
  if (nDropped - m_nDropped > 0)
    NFD_LOG_FACE_DEBUG("Detected " << nDropped - m_nDropped << " dropped packet(s)");
  m_nDropped = nDropped;
#endif

  m_socket.async_read_some(boost::asio::null_buffers(),
                           bind(&EthernetTransport::handleRead, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
}

void
EthernetTransport::processIncomingPacket(const uint8_t* packet, size_t length)
{
  if (length < ethernet::HDR_LEN + ethernet::MIN_DATA_LEN) {
    NFD_LOG_FACE_WARN("Received frame is too short (" << length << " bytes)");
    return;
  }

  const ether_header* eh = reinterpret_cast<const ether_header*>(packet);
  const ethernet::Address sourceAddress(eh->ether_shost);

  // in some cases VLAN-tagged frames may survive the BPF filter,
  // make sure we do not process those frames (see #3348)
  if (ntohs(eh->ether_type) != ethernet::ETHERTYPE_NDN)
    return;

  // check that our BPF filter is working correctly
  BOOST_ASSERT_MSG(ethernet::Address(eh->ether_dhost) == m_destAddress,
                   "Received frame addressed to another host or multicast group");
  BOOST_ASSERT_MSG(sourceAddress != m_srcAddress,
                   "Received frame sent by this host");

  packet += ethernet::HDR_LEN;
  length -= ethernet::HDR_LEN;

  bool isOk = false;
  Block element;
  std::tie(isOk, element) = Block::fromBuffer(packet, length);
  if (!isOk) {
    NFD_LOG_FACE_WARN("Received invalid packet from " << sourceAddress.toString());
    return;
  }

  NFD_LOG_FACE_TRACE("Received: " << element.size() << " bytes from " << sourceAddress.toString());

  Transport::Packet tp(std::move(element));
  static_assert(sizeof(tp.remoteEndpoint) >= ethernet::ADDR_LEN,
                "Transport::Packet::remoteEndpoint is too small");
  std::memcpy(&tp.remoteEndpoint, sourceAddress.data(), sourceAddress.size());
  this->receive(std::move(tp));
}

void
EthernetTransport::processErrorCode(const boost::system::error_code& error)
{
  // boost::asio::error::operation_aborted must be checked first. In that situation, the Transport
  // may already have been destructed, and it's unsafe to call getState() or do logging.
  if (error == boost::asio::error::operation_aborted ||
      getState() == TransportState::CLOSING ||
      getState() == TransportState::FAILED ||
      getState() == TransportState::CLOSED) {
    // transport is shutting down, ignore any errors
    return;
  }

  NFD_LOG_FACE_WARN("Receive operation failed: " << error.message());
}

int
EthernetTransport::getInterfaceMtu()
{
#ifdef SIOCGIFMTU
#if defined(__APPLE__) || defined(__FreeBSD__)
  // see bug #2328
  using boost::asio::ip::udp;
  udp::socket sock(getGlobalIoService(), udp::v4());
  int fd = sock.native_handle();
#else
  int fd = m_socket.native_handle();
#endif

  ifreq ifr{};
  std::strncpy(ifr.ifr_name, m_interfaceName.data(), sizeof(ifr.ifr_name) - 1);

  if (::ioctl(fd, SIOCGIFMTU, &ifr) == 0) {
    NFD_LOG_FACE_DEBUG("Interface MTU is " << ifr.ifr_mtu);
    return ifr.ifr_mtu;
  }

  NFD_LOG_FACE_WARN("Failed to get interface MTU: " << std::strerror(errno));
#endif

  NFD_LOG_FACE_DEBUG("Assuming default MTU of " << ethernet::MAX_DATA_LEN);
  return ethernet::MAX_DATA_LEN;
}

} // namespace face
} // namespace nfd
