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
 **/

#include "ethernet-face.hpp"
#include "core/logger.hpp"
#include "core/network-interface.hpp"

#include <pcap/pcap.h>

#include <cstring>        // for std::strncpy()
#include <arpa/inet.h>    // for htons() and ntohs()
#include <net/ethernet.h> // for struct ether_header
#include <net/if.h>       // for struct ifreq
#include <stdio.h>        // for snprintf()
#include <sys/ioctl.h>    // for ioctl()
#include <unistd.h>       // for dup()

#if !defined(PCAP_NETMASK_UNKNOWN)
/*
 * Value to pass to pcap_compile() as the netmask if you don't know what
 * the netmask is.
 */
#define PCAP_NETMASK_UNKNOWN    0xffffffff
#endif

namespace nfd {

NFD_LOG_INIT("EthernetFace");

EthernetFace::EthernetFace(const shared_ptr<boost::asio::posix::stream_descriptor>& socket,
                           const shared_ptr<NetworkInterfaceInfo>& interface,
                           const ethernet::Address& address)
  : Face(FaceUri(address), FaceUri::fromDev(interface->name))
  , m_socket(socket)
  , m_interfaceName(interface->name)
  , m_srcAddress(interface->etherAddress)
  , m_destAddress(address)
{
  NFD_LOG_INFO("Creating ethernet face on " << m_interfaceName << ": "
               << m_srcAddress << " <--> " << m_destAddress);
  pcapInit();

  int fd = pcap_get_selectable_fd(m_pcap);
  if (fd < 0)
    throw Error("pcap_get_selectable_fd() failed");

  // need to duplicate the fd, otherwise both pcap_close()
  // and stream_descriptor::close() will try to close the
  // same fd and one of them will fail
  m_socket->assign(::dup(fd));

  m_interfaceMtu = getInterfaceMtu();
  NFD_LOG_DEBUG("[id:" << getId() << ",endpoint:" << m_interfaceName
                << "] Interface MTU is: " << m_interfaceMtu);

  char filter[100];
  ::snprintf(filter, sizeof(filter),
             "(ether proto 0x%x) && (ether dst %s) && (not ether src %s)",
             ETHERTYPE_NDN,
             m_destAddress.toString().c_str(),
             m_srcAddress.toString().c_str());
  setPacketFilter(filter);

  m_socket->async_read_some(boost::asio::null_buffers(),
                            bind(&EthernetFace::handleRead, this,
                                 boost::asio::placeholders::error,
                                 boost::asio::placeholders::bytes_transferred));
}

EthernetFace::~EthernetFace()
{
  onFail.clear(); // no reason to call onFail anymore
  close();
}

void
EthernetFace::sendInterest(const Interest& interest)
{
  onSendInterest(interest);
  sendPacket(interest.wireEncode());
}

void
EthernetFace::sendData(const Data& data)
{
  onSendData(data);
  sendPacket(data.wireEncode());
}

void
EthernetFace::close()
{
  if (m_pcap)
    {
      boost::system::error_code error;
      m_socket->close(error); // ignore errors
      pcap_close(m_pcap);
      m_pcap = 0;

      fail("Face closed");
    }
}

void
EthernetFace::pcapInit()
{
  char errbuf[PCAP_ERRBUF_SIZE];
  errbuf[0] = '\0';
  m_pcap = pcap_create(m_interfaceName.c_str(), errbuf);
  if (!m_pcap)
    throw Error("pcap_create(): " + std::string(errbuf));

#ifdef HAVE_PCAP_SET_IMMEDIATE_MODE
  // Enable "immediate mode", effectively disabling any read buffering in the kernel.
  // This corresponds to the BIOCIMMEDIATE ioctl on BSD-like systems (including OS X)
  // where libpcap uses a BPF device. On Linux this forces libpcap not to use TPACKET_V3,
  // even if the kernel supports it, thus preventing bug #1511.
  pcap_set_immediate_mode(m_pcap, 1);
#endif

  /// \todo Do not rely on promisc mode, see task #1278
  if (!m_destAddress.isBroadcast())
    pcap_set_promisc(m_pcap, 1);

  if (pcap_activate(m_pcap) < 0)
    throw Error("pcap_activate() failed");

  if (pcap_set_datalink(m_pcap, DLT_EN10MB) < 0)
    throw Error("pcap_set_datalink(): " + std::string(pcap_geterr(m_pcap)));

  if (pcap_setdirection(m_pcap, PCAP_D_IN) < 0)
    // no need to throw on failure, BPF will filter unwanted packets anyway
    NFD_LOG_WARN("pcap_setdirection(): " << pcap_geterr(m_pcap));
}

void
EthernetFace::setPacketFilter(const char* filterString)
{
  bpf_program filter;
  if (pcap_compile(m_pcap, &filter, filterString, 1, PCAP_NETMASK_UNKNOWN) < 0)
    throw Error("pcap_compile(): " + std::string(pcap_geterr(m_pcap)));

  int ret = pcap_setfilter(m_pcap, &filter);
  pcap_freecode(&filter);
  if (ret < 0)
    throw Error("pcap_setfilter(): " + std::string(pcap_geterr(m_pcap)));
}

void
EthernetFace::sendPacket(const ndn::Block& block)
{
  if (!m_pcap)
    {
      NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interfaceName
                   << "] Trying to send on closed face");
      fail("Face closed");
      return;
    }

  /// \todo Fragmentation
  if (block.size() > m_interfaceMtu)
    {
      NFD_LOG_ERROR("[id:" << getId() << ",endpoint:" << m_interfaceName
                    << "] Fragmentation not implemented: dropping packet larger than MTU");
      return;
    }

  /// \todo Right now there is no reserve when packet is received, but
  ///       we should reserve some space at the beginning and at the end
  ndn::EncodingBuffer buffer(block);

  // pad with zeroes if the payload is too short
  if (block.size() < ethernet::MIN_DATA_LEN)
    {
      static const uint8_t padding[ethernet::MIN_DATA_LEN] = {0};
      buffer.appendByteArray(padding, ethernet::MIN_DATA_LEN - block.size());
    }

  // construct and prepend the ethernet header
  static uint16_t ethertype = htons(ETHERTYPE_NDN);
  buffer.prependByteArray(reinterpret_cast<const uint8_t*>(&ethertype), ethernet::TYPE_LEN);
  buffer.prependByteArray(m_srcAddress.data(), m_srcAddress.size());
  buffer.prependByteArray(m_destAddress.data(), m_destAddress.size());

  // send the packet
  int sent = pcap_inject(m_pcap, buffer.buf(), buffer.size());
  if (sent < 0)
    {
      throw Error("pcap_inject(): " + std::string(pcap_geterr(m_pcap)));
    }
  else if (static_cast<size_t>(sent) < buffer.size())
    {
      throw Error("Failed to send packet");
    }

  NFD_LOG_TRACE("[id:" << getId() << ",endpoint:" << m_interfaceName
                << "] Successfully sent: " << block.size() << " bytes");
  this->getMutableCounters().getNOutBytes() += block.size();
}

void
EthernetFace::handleRead(const boost::system::error_code& error, size_t)
{
  if (error)
    return processErrorCode(error);

  pcap_pkthdr* pktHeader;
  const uint8_t* packet;
  int ret = pcap_next_ex(m_pcap, &pktHeader, &packet);
  if (ret < 0)
    {
      throw Error("pcap_next_ex(): " + std::string(pcap_geterr(m_pcap)));
    }
  else if (ret == 0)
    {
      NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interfaceName
                   << "] pcap_next_ex() timed out");
    }
  else
    {
      size_t length = pktHeader->caplen;
      if (length < ethernet::HDR_LEN + ethernet::MIN_DATA_LEN)
        throw Error("Received packet is too short");

      const ether_header* eh = reinterpret_cast<const ether_header*>(packet);
      if (ntohs(eh->ether_type) != ETHERTYPE_NDN)
        throw Error("Unrecognized ethertype");

      packet += ethernet::HDR_LEN;
      length -= ethernet::HDR_LEN;

      /// \todo Reserve space in front and at the back
      ///       of the underlying buffer
      Block element;
      bool isOk = Block::fromBuffer(packet, length, element);
      if (isOk)
        {
          NFD_LOG_TRACE("[id:" << getId() << ",endpoint:" << m_interfaceName
                        << "] Received: " << element.size() << " bytes");
          this->getMutableCounters().getNInBytes() += element.size();

          if (!decodeAndDispatchInput(element))
            {
              NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interfaceName
                           << "] Received unrecognized block of type " << element.type());
              // ignore unknown packet
            }
        }
      else
        {
          NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interfaceName
                       << "] Received block is invalid or too large to process");
        }
    }

  m_socket->async_read_some(boost::asio::null_buffers(),
                            bind(&EthernetFace::handleRead, this,
                                 boost::asio::placeholders::error,
                                 boost::asio::placeholders::bytes_transferred));
}

void
EthernetFace::processErrorCode(const boost::system::error_code& error)
{
  if (error == boost::system::errc::operation_canceled)
    // when socket is closed by someone
    return;

  if (!m_pcap)
    {
      fail("Face closed");
      return;
    }

  std::string msg;
  if (error == boost::asio::error::eof)
    {
      msg = "Face closed";
      NFD_LOG_INFO("[id:" << getId() << ",endpoint:" << m_interfaceName << "] " << msg);
    }
  else
    {
      msg = "Receive operation failed, closing face: " + error.message();
      NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interfaceName << "] " << msg);
    }

  close();
  fail(msg);
}

size_t
EthernetFace::getInterfaceMtu() const
{
  size_t mtu = ethernet::MAX_DATA_LEN;

#ifdef SIOCGIFMTU
  ifreq ifr = boost::initialized_value;
  std::strncpy(ifr.ifr_name, m_interfaceName.c_str(), sizeof(ifr.ifr_name) - 1);

  if (::ioctl(m_socket->native_handle(), SIOCGIFMTU, &ifr) < 0)
    {
      NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interfaceName
                   << "] Failed to get interface MTU, assuming " << mtu);
    }
  else
    {
      mtu = std::min(mtu, static_cast<size_t>(ifr.ifr_mtu));
    }
#endif

  return mtu;
}

} // namespace nfd
