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

#include "multicast-udp-transport.hpp"
#include "socket-utils.hpp"
#include "udp-protocol.hpp"

#include "core/privilege-helper.hpp"

#include <boost/functional/hash.hpp>

#ifdef __linux__
#include <cerrno>       // for errno
#include <cstring>      // for std::strerror()
#include <sys/socket.h> // for setsockopt()
#endif // __linux__

namespace nfd {
namespace face {

NFD_LOG_INCLASS_2TEMPLATE_SPECIALIZATION_DEFINE(DatagramTransport, MulticastUdpTransport::protocol,
                                                Multicast, "MulticastUdpTransport");

MulticastUdpTransport::MulticastUdpTransport(const protocol::endpoint& multicastGroup,
                                             protocol::socket&& recvSocket,
                                             protocol::socket&& sendSocket,
                                             ndn::nfd::LinkType linkType)
  : DatagramTransport(std::move(recvSocket))
  , m_multicastGroup(multicastGroup)
  , m_sendSocket(std::move(sendSocket))
{
  this->setLocalUri(FaceUri(m_sendSocket.local_endpoint()));
  this->setRemoteUri(FaceUri(multicastGroup));
  this->setScope(ndn::nfd::FACE_SCOPE_NON_LOCAL);
  this->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  this->setLinkType(linkType);
  this->setMtu(udp::computeMtu(m_sendSocket.local_endpoint()));

  protocol::socket::send_buffer_size sendBufferSizeOption;
  boost::system::error_code error;
  m_sendSocket.get_option(sendBufferSizeOption);
  if (error) {
    NFD_LOG_FACE_WARN("Failed to obtain send queue capacity from socket: " << error.message());
    this->setSendQueueCapacity(QUEUE_ERROR);
  }
  else {
    this->setSendQueueCapacity(sendBufferSizeOption.value());
  }

  NFD_LOG_FACE_INFO("Creating transport");
}

ssize_t
MulticastUdpTransport::getSendQueueLength()
{
  ssize_t queueLength = getTxQueueLength(m_sendSocket.native_handle());
  if (queueLength == QUEUE_ERROR) {
    NFD_LOG_FACE_WARN("Failed to obtain send queue length from socket: " << std::strerror(errno));
  }
  return queueLength;
}

void
MulticastUdpTransport::doSend(Transport::Packet&& packet)
{
  NFD_LOG_FACE_TRACE(__func__);

  m_sendSocket.async_send_to(boost::asio::buffer(packet.packet), m_multicastGroup,
                             bind(&MulticastUdpTransport::handleSend, this,
                                  boost::asio::placeholders::error,
                                  boost::asio::placeholders::bytes_transferred,
                                  packet.packet));
}

void
MulticastUdpTransport::doClose()
{
  if (m_sendSocket.is_open()) {
    NFD_LOG_FACE_TRACE("Closing sending socket");

    // Cancel all outstanding operations and close the socket.
    // Use the non-throwing variants and ignore errors, if any.
    boost::system::error_code error;
    m_sendSocket.cancel(error);
    m_sendSocket.close(error);
  }

  DatagramTransport::doClose();
}

static void
bindToDevice(int fd, const std::string& ifname)
{
  // On Linux, if there is more than one MulticastUdpTransport for the same multicast
  // group but they are on different network interfaces, each socket needs to be bound
  // to the corresponding interface using SO_BINDTODEVICE, otherwise the transport will
  // receive all packets sent to the other interfaces as well.
  // This is needed only on Linux. On macOS, the boost::asio::ip::multicast::join_group
  // option is sufficient to obtain the desired behavior.

#ifdef __linux__
  PrivilegeHelper::runElevated([=] {
    if (::setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname.data(), ifname.size() + 1) < 0) {
      BOOST_THROW_EXCEPTION(MulticastUdpTransport::Error("Cannot bind multicast rx socket to " +
                                                         ifname + ": " + std::strerror(errno)));
    }
  });
#endif // __linux__
}

void
MulticastUdpTransport::openRxSocket(protocol::socket& sock,
                                    const protocol::endpoint& multicastGroup,
                                    const boost::asio::ip::address& localAddress,
                                    const shared_ptr<const ndn::net::NetworkInterface>& netif)
{
  BOOST_ASSERT(!sock.is_open());

  sock.open(multicastGroup.protocol());
  sock.set_option(protocol::socket::reuse_address(true));

  if (multicastGroup.address().is_v4()) {
    BOOST_ASSERT(localAddress.is_v4());
    sock.bind(multicastGroup);
    sock.set_option(boost::asio::ip::multicast::join_group(multicastGroup.address().to_v4(),
                                                           localAddress.to_v4()));
  }
  else {
    BOOST_ASSERT(localAddress.is_v6());
    sock.set_option(boost::asio::ip::v6_only(true));
#ifdef WITH_TESTS
    // To simplify unit tests, we bind to the "any" IPv6 address if the supplied multicast
    // address lacks a scope id. Calling bind() without a scope id would otherwise fail.
    if (multicastGroup.address().to_v6().scope_id() == 0)
      sock.bind(protocol::endpoint(boost::asio::ip::address_v6::any(), multicastGroup.port()));
    else
#endif
      sock.bind(multicastGroup);
    sock.set_option(boost::asio::ip::multicast::join_group(multicastGroup.address().to_v6()));
  }

  if (netif)
    bindToDevice(sock.native_handle(), netif->getName());
}

void
MulticastUdpTransport::openTxSocket(protocol::socket& sock,
                                    const protocol::endpoint& localEndpoint,
                                    const shared_ptr<const ndn::net::NetworkInterface>& netif,
                                    bool enableLoopback)
{
  BOOST_ASSERT(!sock.is_open());

  sock.open(localEndpoint.protocol());
  sock.set_option(protocol::socket::reuse_address(true));
  sock.set_option(boost::asio::ip::multicast::enable_loopback(enableLoopback));

  if (localEndpoint.address().is_v4()) {
    sock.bind(localEndpoint);
    if (!localEndpoint.address().is_unspecified())
      sock.set_option(boost::asio::ip::multicast::outbound_interface(localEndpoint.address().to_v4()));
  }
  else {
    sock.set_option(boost::asio::ip::v6_only(true));
    sock.bind(localEndpoint);
    if (netif)
      sock.set_option(boost::asio::ip::multicast::outbound_interface(netif->getIndex()));
  }
}

template<>
Transport::EndpointId
DatagramTransport<boost::asio::ip::udp, Multicast>::makeEndpointId(const protocol::endpoint& ep)
{
  if (ep.address().is_v4()) {
    return (static_cast<uint64_t>(ep.port()) << 32) |
            static_cast<uint64_t>(ep.address().to_v4().to_ulong());
  }
  else {
    size_t seed = 0;
    const auto& addrBytes = ep.address().to_v6().to_bytes();
    boost::hash_range(seed, addrBytes.begin(), addrBytes.end());
    boost::hash_combine(seed, ep.port());
    return seed;
  }
}

} // namespace face
} // namespace nfd
