/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "ethernet-face.hpp"

#include <pcap/pcap.h>

#include <arpa/inet.h>    // for htons() and ntohs()
#include <net/ethernet.h> // for struct ether_header
#include <net/if.h>       // for struct ifreq
#include <stdio.h>        // for snprintf()
#include <sys/ioctl.h>    // for ioctl()
#include <unistd.h>       // for dup()

#ifndef SIOCGIFHWADDR
#include <net/if_dl.h>    // for struct sockaddr_dl
// must be included *after* <net/if.h>
#include <ifaddrs.h>      // for getifaddrs()
#endif

namespace nfd {

NFD_LOG_INIT("EthernetFace")

static const uint8_t MAX_PADDING[ethernet::MIN_DATA_LEN] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


EthernetFace::EthernetFace(const shared_ptr<boost::asio::posix::stream_descriptor>& socket,
                           const ethernet::Endpoint& interface,
                           const ethernet::Address& address)
  : m_socket(socket)
  , m_interface(interface)
  , m_destAddress(address)
{
  setLocal(false);
  pcapInit();

  int fd = pcap_get_selectable_fd(m_pcap);
  if (fd < 0)
    throw Error("pcap_get_selectable_fd() failed");

  // need to duplicate the fd, otherwise both pcap_close()
  // and stream_descriptor::close() will try to close the
  // same fd and one of them will fail
  m_socket->assign(::dup(fd));

  m_sourceAddress = getInterfaceAddress();
  NFD_LOG_DEBUG("[id:" << getId() << ",endpoint:" << m_interface
                << "] Local MAC address is: " << m_sourceAddress);
  m_interfaceMtu = getInterfaceMtu();
  NFD_LOG_DEBUG("[id:" << getId() << ",endpoint:" << m_interface
                << "] Interface MTU is: " << m_interfaceMtu);

  char filter[100];
  ::snprintf(filter, sizeof(filter),
             "(ether proto 0x%x) && (ether dst %s) && (not ether src %s)",
             ETHERTYPE_NDN,
             m_destAddress.toString(':').c_str(),
             m_sourceAddress.toString(':').c_str());
  setPacketFilter(filter);

  m_socket->async_read_some(boost::asio::null_buffers(),
                            bind(&EthernetFace::handleRead, this,
                                 boost::asio::placeholders::error,
                                 boost::asio::placeholders::bytes_transferred));
}

EthernetFace::~EthernetFace()
{
  close();
}

void
EthernetFace::sendInterest(const Interest& interest)
{
  sendPacket(interest.wireEncode());
}

void
EthernetFace::sendData(const Data& data)
{
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
    }
}

void
EthernetFace::pcapInit()
{
  NFD_LOG_TRACE("[id:" << getId() << ",endpoint:" << m_interface
                << "] Initializing pcap");

  char errbuf[PCAP_ERRBUF_SIZE];
  errbuf[0] = '\0';
  m_pcap = pcap_create(m_interface.c_str(), errbuf);
  if (!m_pcap)
    throw Error("pcap_create(): " + std::string(errbuf));

  if (pcap_activate(m_pcap) < 0)
    throw Error("pcap_activate() failed");

  errbuf[0] = '\0';
  if (pcap_setnonblock(m_pcap, 1, errbuf) < 0)
    throw Error("pcap_setnonblock(): " + std::string(errbuf));

  if (pcap_setdirection(m_pcap, PCAP_D_IN) < 0)
    throw Error("pcap_setdirection(): " + std::string(pcap_geterr(m_pcap)));

  if (pcap_set_datalink(m_pcap, DLT_EN10MB) < 0)
    throw Error("pcap_set_datalink(): " + std::string(pcap_geterr(m_pcap)));
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
      NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interface
                   << "] Trying to send on closed face");
      onFail("Face closed");
      return;
    }

  /// @todo Fragmentation
  if (block.size() > m_interfaceMtu)
    throw Error("Fragmentation not implemented");

  /// \todo Right now there is no reserve when packet is received, but
  ///       we should reserve some space at the beginning and at the end
  ndn::EncodingBuffer buffer(block);

  if (block.size() < ethernet::MIN_DATA_LEN)
    {
      buffer.appendByteArray(MAX_PADDING, ethernet::MIN_DATA_LEN - block.size());
    }

  // construct and prepend the ethernet header
  static uint16_t ethertype = htons(ETHERTYPE_NDN);
  buffer.prependByteArray(reinterpret_cast<const uint8_t*>(&ethertype), ethernet::TYPE_LEN);
  buffer.prependByteArray(m_sourceAddress.data(), m_sourceAddress.size());
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

  NFD_LOG_TRACE("[id:" << getId() << ",endpoint:" << m_interface
                << "] Successfully sent: " << buffer.size() << " bytes");
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
      NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interface
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
      NFD_LOG_TRACE("[id:" << getId() << ",endpoint:" << m_interface
                    << "] Received: " << length << " bytes");

      /// \todo Eliminate reliance on exceptions in this path
      try {
        /// \todo Reserve space in front and at the back
        ///       of the underlying buffer
        ndn::Block element(packet, length);
        if (!decodeAndDispatchInput(element))
          {
            NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interface
                         << "] Received unrecognized block of type ["
                         << element.type() << "]");
            // ignore unknown packet
          }
      } catch (const tlv::Error&) {
        NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interface
                     << "] Received input is invalid or too large to process");
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
      onFail("Face closed");
      return;
    }

  std::string msg;
  if (error == boost::asio::error::eof)
    {
      msg = "Face closed";
      NFD_LOG_INFO("[id:" << getId() << ",endpoint:" << m_interface << "] " << msg);
    }
  else
    {
      msg = "Send or receive operation failed, closing face: "
          + error.category().message(error.value());
      NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interface << "] " << msg);
    }

  close();
  onFail(msg);
}

ethernet::Address
EthernetFace::getInterfaceAddress() const
{
#ifdef SIOCGIFHWADDR
  ifreq ifr = {};
  ::strncpy(ifr.ifr_name, m_interface.c_str(), sizeof(ifr.ifr_name));

  if (::ioctl(m_socket->native_handle(), SIOCGIFHWADDR, &ifr) < 0)
    throw Error("ioctl(SIOCGIFHWADDR) failed");

  uint8_t* hwaddr = reinterpret_cast<uint8_t*>(ifr.ifr_hwaddr.sa_data);
  return ethernet::Address(hwaddr);
#else
  ifaddrs* addrlist;
  if (::getifaddrs(&addrlist) < 0)
    throw Error("getifaddrs() failed");

  ethernet::Address address;
  for (ifaddrs* ifa = addrlist; ifa != 0; ifa = ifa->ifa_next)
    {
      if (std::string(ifa->ifa_name) == m_interface
          && ifa->ifa_addr != 0
          && ifa->ifa_addr->sa_family == AF_LINK)
        {
          sockaddr_dl* sa = reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr);
          address = ethernet::Address(reinterpret_cast<uint8_t*>(LLADDR(sa)));
          break;
        }
    }

  ::freeifaddrs(addrlist);
  return address;
#endif
}

size_t
EthernetFace::getInterfaceMtu() const
{
  size_t mtu = ethernet::MAX_DATA_LEN;

#ifdef SIOCGIFMTU
  ifreq ifr = {};
  ::strncpy(ifr.ifr_name, m_interface.c_str(), sizeof(ifr.ifr_name));

  if (::ioctl(m_socket->native_handle(), SIOCGIFMTU, &ifr) < 0)
    {
      NFD_LOG_WARN("[id:" << getId() << ",endpoint:" << m_interface
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
