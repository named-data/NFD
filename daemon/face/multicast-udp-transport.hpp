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

#ifndef NFD_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_HPP
#define NFD_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_HPP

#include "datagram-transport.hpp"

#include <ndn-cxx/net/network-interface.hpp>

namespace nfd {
namespace face {

// Explicit specialization of makeEndpointId for the UDP multicast case.
// Note that this "shall be declared before the first use of the specialization
// that would cause an implicit instantiation to take place, in every translation
// unit in which such a use occurs".
template<>
Transport::EndpointId
DatagramTransport<boost::asio::ip::udp, Multicast>::makeEndpointId(const protocol::endpoint& ep);

/**
 * \brief A Transport that communicates on a UDP multicast group
 */
class MulticastUdpTransport final : public DatagramTransport<boost::asio::ip::udp, Multicast>
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  /**
   * \brief Creates a UDP-based transport for multicast communication
   * \param multicastGroup multicast group
   * \param recvSocket socket used to receive multicast packets
   * \param sendSocket socket used to send to the multicast group
   * \param linkType either `ndn::nfd::LINK_TYPE_MULTI_ACCESS` or `ndn::nfd::LINK_TYPE_AD_HOC`
   */
  MulticastUdpTransport(const protocol::endpoint& multicastGroup,
                        protocol::socket&& recvSocket,
                        protocol::socket&& sendSocket,
                        ndn::nfd::LinkType linkType);

  ssize_t
  getSendQueueLength() final;

  static void
  openRxSocket(protocol::socket& sock,
               const protocol::endpoint& multicastGroup,
               const boost::asio::ip::address& localAddress,
               const shared_ptr<const ndn::net::NetworkInterface>& netif = nullptr);

  static void
  openTxSocket(protocol::socket& sock,
               const protocol::endpoint& localEndpoint,
               const shared_ptr<const ndn::net::NetworkInterface>& netif = nullptr,
               bool enableLoopback = false);

private:
  void
  doSend(Transport::Packet&& packet) final;

  void
  doClose() final;

private:
  protocol::endpoint m_multicastGroup;
  protocol::socket m_sendSocket;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_HPP
