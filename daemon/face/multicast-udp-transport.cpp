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

#include "multicast-udp-transport.hpp"
#include "udp-protocol.hpp"

namespace nfd {
namespace face {

NFD_LOG_INCLASS_2TEMPLATE_SPECIALIZATION_DEFINE(DatagramTransport, MulticastUdpTransport::protocol,
                                                Multicast, "MulticastUdpTransport");

MulticastUdpTransport::MulticastUdpTransport(const protocol::endpoint& localEndpoint,
                                             const protocol::endpoint& multicastGroup,
                                             protocol::socket&& recvSocket,
                                             protocol::socket&& sendSocket)
  : DatagramTransport(std::move(recvSocket))
  , m_multicastGroup(multicastGroup)
  , m_sendSocket(std::move(sendSocket))
{
  this->setLocalUri(FaceUri(localEndpoint));
  this->setRemoteUri(FaceUri(multicastGroup));
  this->setScope(ndn::nfd::FACE_SCOPE_NON_LOCAL);
  this->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  this->setLinkType(ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  this->setMtu(udp::computeMtu(localEndpoint));

  NFD_LOG_FACE_INFO("Creating transport");
}

void
MulticastUdpTransport::beforeChangePersistency(ndn::nfd::FacePersistency newPersistency)
{
  if (newPersistency != ndn::nfd::FACE_PERSISTENCY_PERMANENT) {
    BOOST_THROW_EXCEPTION(
      std::invalid_argument("MulticastUdpTransport supports only FACE_PERSISTENCY_PERMANENT"));
  }
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

template<>
Transport::EndpointId
DatagramTransport<boost::asio::ip::udp, Multicast>::makeEndpointId(const protocol::endpoint& ep)
{
  // IPv6 multicast is not supported
  BOOST_ASSERT(ep.address().is_v4());

  return (static_cast<uint64_t>(ep.port()) << 32) |
          static_cast<uint64_t>(ep.address().to_v4().to_ulong());
}

} // namespace face
} // namespace nfd
