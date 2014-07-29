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
 */

#include "multicast-udp-face.hpp"

namespace nfd {

NFD_LOG_INCLASS_2TEMPLATE_SPECIALIZATION_DEFINE(DatagramFace,
                                                MulticastUdpFace::protocol, Multicast,
                                                "MulticastUdpFace");

MulticastUdpFace::MulticastUdpFace(const shared_ptr<MulticastUdpFace::protocol::socket>& recvSocket,
                                   const shared_ptr<MulticastUdpFace::protocol::socket>& sendSocket,
                                   const MulticastUdpFace::protocol::endpoint& localEndpoint,
                                   const MulticastUdpFace::protocol::endpoint& multicastEndpoint)
  : DatagramFace<protocol, Multicast>(FaceUri(multicastEndpoint),
                                      FaceUri(localEndpoint),
                                      recvSocket, false)
  , m_multicastGroup(multicastEndpoint)
  , m_sendSocket(sendSocket)
{
  NFD_LOG_INFO("Creating multicast UDP face for group " << m_multicastGroup);
}

const MulticastUdpFace::protocol::endpoint&
MulticastUdpFace::getMulticastGroup() const
{
  return m_multicastGroup;
}

void
MulticastUdpFace::sendBlock(const Block& block)
{
  m_sendSocket->async_send_to(boost::asio::buffer(block.wire(), block.size()),
                              m_multicastGroup,
                              bind(&MulticastUdpFace::handleSend, this, _1, _2, block));
}

void
MulticastUdpFace::sendInterest(const Interest& interest)
{
  onSendInterest(interest);

  NFD_LOG_DEBUG("Sending interest");
  sendBlock(interest.wireEncode());
}

void
MulticastUdpFace::sendData(const Data& data)
{
  /// \todo After this method implements duplicate suppression, onSendData event should
  ///       be triggered only when data is actually sent out
  onSendData(data);

  NFD_LOG_DEBUG("Sending data");
  sendBlock(data.wireEncode());
}

bool
MulticastUdpFace::isMultiAccess() const
{
  return true;
}

} // namespace nfd
