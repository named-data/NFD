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

#include "multicast-ethernet-transport.hpp"
#include "core/global-io.hpp"

#include <cerrno>         // for errno
#include <cstring>        // for memcpy(), strerror(), strncpy()
#include <net/if.h>       // for struct ifreq
#include <stdio.h>        // for snprintf()
#include <sys/ioctl.h>    // for ioctl()

#if defined(__linux__)
#include <netpacket/packet.h> // for struct packet_mreq
#include <sys/socket.h>       // for setsockopt()
#endif

#ifdef SIOCADDMULTI
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_dl.h>    // for struct sockaddr_dl
#endif
#endif

namespace nfd {
namespace face {

NFD_LOG_INIT("MulticastEthernetTransport");

MulticastEthernetTransport::MulticastEthernetTransport(const ndn::net::NetworkInterface& localEndpoint,
                                                       const ethernet::Address& mcastAddress,
                                                       ndn::nfd::LinkType linkType)
  : EthernetTransport(localEndpoint, mcastAddress)
#if defined(__linux__)
  , m_interfaceIndex(localEndpoint.getIndex())
#endif
{
  this->setLocalUri(FaceUri::fromDev(m_interfaceName));
  this->setRemoteUri(FaceUri(m_destAddress));
  this->setScope(ndn::nfd::FACE_SCOPE_NON_LOCAL);
  this->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  this->setLinkType(linkType);
  this->setMtu(localEndpoint.getMtu());

  NFD_LOG_FACE_INFO("Creating transport");

  char filter[110];
  // note #1: we cannot use std::snprintf because it's not available
  //          on some platforms (see #2299)
  // note #2: "not vlan" must appear last in the filter expression, or the
  //          rest of the filter won't work as intended (see pcap-filter(7))
  snprintf(filter, sizeof(filter),
           "(ether proto 0x%x) && (ether dst %s) && (not ether src %s) && (not vlan)",
           ethernet::ETHERTYPE_NDN,
           m_destAddress.toString().data(),
           m_srcAddress.toString().data());
  m_pcap.setPacketFilter(filter);

  BOOST_ASSERT(m_destAddress.isMulticast());
  if (!m_destAddress.isBroadcast())
    joinMulticastGroup();
}

void
MulticastEthernetTransport::joinMulticastGroup()
{
#if defined(__linux__)
  packet_mreq mr{};
  mr.mr_ifindex = m_interfaceIndex;
  mr.mr_type = PACKET_MR_MULTICAST;
  mr.mr_alen = m_destAddress.size();
  std::memcpy(mr.mr_address, m_destAddress.data(), m_destAddress.size());

  if (::setsockopt(m_socket.native_handle(), SOL_PACKET,
                   PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == 0)
    return; // success

  NFD_LOG_FACE_WARN("setsockopt(PACKET_ADD_MEMBERSHIP) failed: " << std::strerror(errno));
#endif

#if defined(SIOCADDMULTI)
  ifreq ifr{};
  std::strncpy(ifr.ifr_name, m_interfaceName.data(), sizeof(ifr.ifr_name) - 1);

#if defined(__APPLE__) || defined(__FreeBSD__)
  // see bug #2327
  using boost::asio::ip::udp;
  udp::socket sock(getGlobalIoService(), udp::v4());
  int fd = sock.native_handle();

  // Differences between Linux and the BSDs (including macOS):
  //   o BSD does not have ifr_hwaddr; use ifr_addr instead.
  //   o While macOS seems to accept both AF_LINK and AF_UNSPEC as the address
  //     family, FreeBSD explicitly requires AF_LINK, so we have to use AF_LINK
  //     and sockaddr_dl instead of the generic sockaddr structure.
  //   o BSD's sockaddr (and sockaddr_dl in particular) contains an additional
  //     field, sa_len (sdl_len), which must be set to the total length of the
  //     structure, including the length field itself.
  //   o We do not specify the interface name, thus sdl_nlen is left at 0 and
  //     LLADDR is effectively the same as sdl_data.

  sockaddr_dl* sdl = reinterpret_cast<sockaddr_dl*>(&ifr.ifr_addr);
  sdl->sdl_len = sizeof(ifr.ifr_addr);
  sdl->sdl_family = AF_LINK;
  sdl->sdl_alen = m_destAddress.size();
  std::memcpy(LLADDR(sdl), m_destAddress.data(), m_destAddress.size());

  static_assert(sizeof(ifr.ifr_addr) >= offsetof(sockaddr_dl, sdl_data) + ethernet::ADDR_LEN,
                "ifr_addr in struct ifreq is too small on this platform");
#else
  int fd = m_socket.native_handle();

  ifr.ifr_hwaddr.sa_family = AF_UNSPEC;
  std::memcpy(ifr.ifr_hwaddr.sa_data, m_destAddress.data(), m_destAddress.size());

  static_assert(sizeof(ifr.ifr_hwaddr.sa_data) >= ethernet::ADDR_LEN,
                "ifr_hwaddr in struct ifreq is too small on this platform");
#endif

  if (::ioctl(fd, SIOCADDMULTI, &ifr) == 0)
    return; // success

  NFD_LOG_FACE_WARN("ioctl(SIOCADDMULTI) failed: " << std::strerror(errno));
#endif

  BOOST_THROW_EXCEPTION(Error("Failed to join multicast group"));
}

} // namespace face
} // namespace nfd
