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

#include "ethernet-face.hpp"
#include "core/global-io.hpp"

#include <pcap/pcap.h>

#include <cerrno>         // for errno
#include <cstring>        // for std::strerror() and std::strncpy()
#include <stdio.h>        // for snprintf()
#include <arpa/inet.h>    // for htons() and ntohs()
#include <net/ethernet.h> // for struct ether_header
#include <net/if.h>       // for struct ifreq
#include <sys/ioctl.h>    // for ioctl()
#include <unistd.h>       // for dup()

#if defined(__linux__)
#include <netpacket/packet.h> // for struct packet_mreq
#include <sys/socket.h>       // for setsockopt()
#endif

#ifdef SIOCADDMULTI
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_dl.h>    // for struct sockaddr_dl
#endif
#endif

#if !defined(PCAP_NETMASK_UNKNOWN)
/*
 * Value to pass to pcap_compile() as the netmask if you don't know what
 * the netmask is.
 */
#define PCAP_NETMASK_UNKNOWN    0xffffffff
#endif

namespace nfd {

NFD_LOG_INIT("EthernetFace");

const time::nanoseconds EthernetFace::REASSEMBLER_LIFETIME = time::seconds(60);

EthernetFace::EthernetFace(boost::asio::posix::stream_descriptor socket,
                           const NetworkInterfaceInfo& interface,
                           const ethernet::Address& address)
  : Face(FaceUri(address), FaceUri::fromDev(interface.name), false, true)
  , m_pcap(nullptr, pcap_close)
  , m_socket(std::move(socket))
#if defined(__linux__)
  , m_interfaceIndex(interface.index)
#endif
  , m_interfaceName(interface.name)
  , m_srcAddress(interface.etherAddress)
  , m_destAddress(address)
#ifdef _DEBUG
  , m_nDropped(0)
#endif
{
  NFD_LOG_FACE_INFO("Creating face on " << m_interfaceName << "/" << m_srcAddress);
  pcapInit();

  int fd = pcap_get_selectable_fd(m_pcap.get());
  if (fd < 0)
    BOOST_THROW_EXCEPTION(Error("pcap_get_selectable_fd failed"));

  // need to duplicate the fd, otherwise both pcap_close()
  // and stream_descriptor::close() will try to close the
  // same fd and one of them will fail
  m_socket.assign(::dup(fd));

  m_interfaceMtu = getInterfaceMtu();
  NFD_LOG_FACE_DEBUG("Interface MTU is: " << m_interfaceMtu);

  m_slicer.reset(new ndnlp::Slicer(m_interfaceMtu));

  char filter[100];
  // std::snprintf not found in some environments
  // http://redmine.named-data.net/issues/2299 for more information
  snprintf(filter, sizeof(filter),
           "(ether proto 0x%x) && (ether dst %s) && (not ether src %s)",
           ethernet::ETHERTYPE_NDN,
           m_destAddress.toString().c_str(),
           m_srcAddress.toString().c_str());
  setPacketFilter(filter);

  if (!m_destAddress.isBroadcast() && !joinMulticastGroup())
    {
      NFD_LOG_FACE_WARN("Falling back to promiscuous mode");
      pcap_set_promisc(m_pcap.get(), 1);
    }

  m_socket.async_read_some(boost::asio::null_buffers(),
                           bind(&EthernetFace::handleRead, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
}

void
EthernetFace::sendInterest(const Interest& interest)
{
  NFD_LOG_FACE_TRACE(__func__);

  this->emitSignal(onSendInterest, interest);

  ndnlp::PacketArray pa = m_slicer->slice(interest.wireEncode());
  for (const auto& packet : *pa) {
    sendPacket(packet);
  }
}

void
EthernetFace::sendData(const Data& data)
{
  NFD_LOG_FACE_TRACE(__func__);

  this->emitSignal(onSendData, data);

  ndnlp::PacketArray pa = m_slicer->slice(data.wireEncode());
  for (const auto& packet : *pa) {
    sendPacket(packet);
  }
}

void
EthernetFace::close()
{
  if (!m_pcap)
    return;

  NFD_LOG_FACE_INFO("Closing face");

  boost::system::error_code error;
  m_socket.cancel(error); // ignore errors
  m_socket.close(error);  // ignore errors
  m_pcap.reset();

  fail("Face closed");
}

void
EthernetFace::pcapInit()
{
  char errbuf[PCAP_ERRBUF_SIZE] = {};
  m_pcap.reset(pcap_create(m_interfaceName.c_str(), errbuf));
  if (!m_pcap)
    BOOST_THROW_EXCEPTION(Error("pcap_create: " + std::string(errbuf)));

#ifdef HAVE_PCAP_SET_IMMEDIATE_MODE
  // Enable "immediate mode", effectively disabling any read buffering in the kernel.
  // This corresponds to the BIOCIMMEDIATE ioctl on BSD-like systems (including OS X)
  // where libpcap uses a BPF device. On Linux this forces libpcap not to use TPACKET_V3,
  // even if the kernel supports it, thus preventing bug #1511.
  pcap_set_immediate_mode(m_pcap.get(), 1);
#endif

  if (pcap_activate(m_pcap.get()) < 0)
    BOOST_THROW_EXCEPTION(Error("pcap_activate failed"));

  if (pcap_set_datalink(m_pcap.get(), DLT_EN10MB) < 0)
    BOOST_THROW_EXCEPTION(Error("pcap_set_datalink: " + std::string(pcap_geterr(m_pcap.get()))));

  if (pcap_setdirection(m_pcap.get(), PCAP_D_IN) < 0)
    // no need to throw on failure, BPF will filter unwanted packets anyway
    NFD_LOG_FACE_WARN("pcap_setdirection failed: " << pcap_geterr(m_pcap.get()));
}

void
EthernetFace::setPacketFilter(const char* filterString)
{
  bpf_program filter;
  if (pcap_compile(m_pcap.get(), &filter, filterString, 1, PCAP_NETMASK_UNKNOWN) < 0)
    BOOST_THROW_EXCEPTION(Error("pcap_compile: " + std::string(pcap_geterr(m_pcap.get()))));

  int ret = pcap_setfilter(m_pcap.get(), &filter);
  pcap_freecode(&filter);
  if (ret < 0)
    BOOST_THROW_EXCEPTION(Error("pcap_setfilter: " + std::string(pcap_geterr(m_pcap.get()))));
}

bool
EthernetFace::joinMulticastGroup()
{
#if defined(__linux__)
  packet_mreq mr{};
  mr.mr_ifindex = m_interfaceIndex;
  mr.mr_type = PACKET_MR_MULTICAST;
  mr.mr_alen = m_destAddress.size();
  std::copy(m_destAddress.begin(), m_destAddress.end(), mr.mr_address);

  if (::setsockopt(m_socket.native_handle(), SOL_PACKET,
                   PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == 0)
    return true; // success

  NFD_LOG_FACE_WARN("setsockopt(PACKET_ADD_MEMBERSHIP) failed: " << std::strerror(errno));
#endif

#if defined(SIOCADDMULTI)
  ifreq ifr{};
  std::strncpy(ifr.ifr_name, m_interfaceName.c_str(), sizeof(ifr.ifr_name) - 1);

#if defined(__APPLE__) || defined(__FreeBSD__)
  // see bug #2327
  using boost::asio::ip::udp;
  udp::socket sock(ref(getGlobalIoService()), udp::v4());
  int fd = sock.native_handle();

  /*
   * Differences between Linux and the BSDs (including OS X):
   *   o BSD does not have ifr_hwaddr; use ifr_addr instead.
   *   o While OS X seems to accept both AF_LINK and AF_UNSPEC as the address
   *     family, FreeBSD explicitly requires AF_LINK, so we have to use AF_LINK
   *     and sockaddr_dl instead of the generic sockaddr structure.
   *   o BSD's sockaddr (and sockaddr_dl in particular) contains an additional
   *     field, sa_len (sdl_len), which must be set to the total length of the
   *     structure, including the length field itself.
   *   o We do not specify the interface name, thus sdl_nlen is left at 0 and
   *     LLADDR is effectively the same as sdl_data.
   */
  sockaddr_dl* sdl = reinterpret_cast<sockaddr_dl*>(&ifr.ifr_addr);
  sdl->sdl_len = sizeof(ifr.ifr_addr);
  sdl->sdl_family = AF_LINK;
  sdl->sdl_alen = m_destAddress.size();
  std::copy(m_destAddress.begin(), m_destAddress.end(), LLADDR(sdl));

  static_assert(sizeof(ifr.ifr_addr) >= offsetof(sockaddr_dl, sdl_data) + ethernet::ADDR_LEN,
                "ifr_addr in struct ifreq is too small on this platform");
#else
  int fd = m_socket.native_handle();

  ifr.ifr_hwaddr.sa_family = AF_UNSPEC;
  std::copy(m_destAddress.begin(), m_destAddress.end(), ifr.ifr_hwaddr.sa_data);

  static_assert(sizeof(ifr.ifr_hwaddr) >= offsetof(sockaddr, sa_data) + ethernet::ADDR_LEN,
                "ifr_hwaddr in struct ifreq is too small on this platform");
#endif

  if (::ioctl(fd, SIOCADDMULTI, &ifr) == 0)
    return true; // success

  NFD_LOG_FACE_WARN("ioctl(SIOCADDMULTI) failed: " << std::strerror(errno));
#endif

  return false;
}

void
EthernetFace::sendPacket(const ndn::Block& block)
{
  if (!m_pcap)
    {
      NFD_LOG_FACE_WARN("Trying to send on closed face");
      return fail("Face closed");
    }

  BOOST_ASSERT(block.size() <= m_interfaceMtu);

  /// \todo Right now there is no reserve when packet is received, but
  ///       we should reserve some space at the beginning and at the end
  ndn::EncodingBuffer buffer(block);

  // pad with zeroes if the payload is too short
  if (block.size() < ethernet::MIN_DATA_LEN)
    {
      static const uint8_t padding[ethernet::MIN_DATA_LEN] = {};
      buffer.appendByteArray(padding, ethernet::MIN_DATA_LEN - block.size());
    }

  // construct and prepend the ethernet header
  static uint16_t ethertype = htons(ethernet::ETHERTYPE_NDN);
  buffer.prependByteArray(reinterpret_cast<const uint8_t*>(&ethertype), ethernet::TYPE_LEN);
  buffer.prependByteArray(m_srcAddress.data(), m_srcAddress.size());
  buffer.prependByteArray(m_destAddress.data(), m_destAddress.size());

  // send the packet
  int sent = pcap_inject(m_pcap.get(), buffer.buf(), buffer.size());
  if (sent < 0)
    {
      return fail("pcap_inject: " + std::string(pcap_geterr(m_pcap.get())));
    }
  else if (static_cast<size_t>(sent) < buffer.size())
    {
      return fail("Failed to inject frame");
    }

  NFD_LOG_FACE_TRACE("Successfully sent: " << block.size() << " bytes");
  this->getMutableCounters().getNOutBytes() += block.size();
}

void
EthernetFace::handleRead(const boost::system::error_code& error, size_t)
{
  if (!m_pcap)
    return fail("Face closed");

  if (error)
    return processErrorCode(error);

  pcap_pkthdr* header;
  const uint8_t* packet;
  int ret = pcap_next_ex(m_pcap.get(), &header, &packet);
  if (ret < 0)
    {
      return fail("pcap_next_ex: " + std::string(pcap_geterr(m_pcap.get())));
    }
  else if (ret == 0)
    {
      NFD_LOG_FACE_WARN("Read timeout");
    }
  else
    {
      processIncomingPacket(header, packet);
    }

#ifdef _DEBUG
  pcap_stat ps{};
  ret = pcap_stats(m_pcap.get(), &ps);
  if (ret < 0)
    {
      NFD_LOG_FACE_DEBUG("pcap_stats failed: " << pcap_geterr(m_pcap.get()));
    }
  else if (ret == 0)
    {
      if (ps.ps_drop - m_nDropped > 0)
        NFD_LOG_FACE_DEBUG("Detected " << ps.ps_drop - m_nDropped << " dropped packet(s)");
      m_nDropped = ps.ps_drop;
    }
#endif

  m_socket.async_read_some(boost::asio::null_buffers(),
                           bind(&EthernetFace::handleRead, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
}

void
EthernetFace::processIncomingPacket(const pcap_pkthdr* header, const uint8_t* packet)
{
  size_t length = header->caplen;
  if (length < ethernet::HDR_LEN + ethernet::MIN_DATA_LEN) {
    NFD_LOG_FACE_WARN("Received frame is too short (" << length << " bytes)");
    return;
  }

  const ether_header* eh = reinterpret_cast<const ether_header*>(packet);
  const ethernet::Address sourceAddress(eh->ether_shost);

  // assert in case BPF fails to filter unwanted frames
  BOOST_ASSERT_MSG(ethernet::Address(eh->ether_dhost) == m_destAddress,
                   "Received frame addressed to a different multicast group");
  BOOST_ASSERT_MSG(sourceAddress != m_srcAddress,
                   "Received frame sent by this host");
  BOOST_ASSERT_MSG(ntohs(eh->ether_type) == ethernet::ETHERTYPE_NDN,
                   "Received frame with unrecognized ethertype");

  packet += ethernet::HDR_LEN;
  length -= ethernet::HDR_LEN;

  /// \todo Reserve space in front and at the back of the underlying buffer
  bool isOk = false;
  Block fragmentBlock;
  std::tie(isOk, fragmentBlock) = Block::fromBuffer(packet, length);
  if (!isOk) {
    NFD_LOG_FACE_WARN("Block received from " << sourceAddress.toString()
                      << " is invalid or too large to process");
    return;
  }

  NFD_LOG_FACE_TRACE("Received: " << fragmentBlock.size() << " bytes from "
                     << sourceAddress.toString());
  this->getMutableCounters().getNInBytes() += fragmentBlock.size();

  Reassembler& reassembler = m_reassemblers[sourceAddress];
  if (!reassembler.pms) {
    // new sender, setup a PartialMessageStore for it
    reassembler.pms.reset(new ndnlp::PartialMessageStore);
    reassembler.pms->onReceive.connect(
      [this, sourceAddress] (const Block& block) {
        NFD_LOG_FACE_TRACE("All fragments received from " << sourceAddress.toString());
        if (!decodeAndDispatchInput(block))
          NFD_LOG_FACE_WARN("Received unrecognized TLV block of type " << block.type()
                            << " from " << sourceAddress.toString());
      });
  }

  scheduler::cancel(reassembler.expireEvent);
  reassembler.expireEvent = scheduler::schedule(REASSEMBLER_LIFETIME,
    [this, sourceAddress] {
      BOOST_VERIFY(m_reassemblers.erase(sourceAddress) == 1);
    });

  ndnlp::NdnlpData fragment;
  std::tie(isOk, fragment) = ndnlp::NdnlpData::fromBlock(fragmentBlock);
  if (!isOk) {
    NFD_LOG_FACE_WARN("Received invalid NDNLP fragment from " << sourceAddress.toString());
    return;
  }

  reassembler.pms->receive(fragment);
}

void
EthernetFace::processErrorCode(const boost::system::error_code& error)
{
  if (error == boost::asio::error::operation_aborted)
    // cancel() has been called on the socket
    return;

  std::string msg;
  if (error == boost::asio::error::eof)
    {
      msg = "Face closed";
    }
  else
    {
      msg = "Receive operation failed: " + error.message();
      NFD_LOG_FACE_WARN(msg);
    }
  fail(msg);
}

size_t
EthernetFace::getInterfaceMtu()
{
#ifdef SIOCGIFMTU
#if defined(__APPLE__) || defined(__FreeBSD__)
  // see bug #2328
  using boost::asio::ip::udp;
  udp::socket sock(ref(getGlobalIoService()), udp::v4());
  int fd = sock.native_handle();
#else
  int fd = m_socket.native_handle();
#endif

  ifreq ifr{};
  std::strncpy(ifr.ifr_name, m_interfaceName.c_str(), sizeof(ifr.ifr_name) - 1);

  if (::ioctl(fd, SIOCGIFMTU, &ifr) == 0)
    return static_cast<size_t>(ifr.ifr_mtu);

  NFD_LOG_FACE_WARN("Failed to get interface MTU: " << std::strerror(errno));
#endif

  return ethernet::MAX_DATA_LEN;
}

} // namespace nfd
