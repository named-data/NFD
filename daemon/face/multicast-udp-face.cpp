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

#include "multicast-udp-face.hpp"

namespace nfd {

NFD_LOG_INCLASS_2TEMPLATE_SPECIALIZATION_DEFINE(DatagramFace,
                                                MulticastUdpFace::protocol, Multicast,
                                                "MulticastUdpFace");

MulticastUdpFace::MulticastUdpFace(const protocol::endpoint& multicastGroup,
                                   const FaceUri& localUri,
                                   protocol::socket recvSocket, protocol::socket sendSocket)
  : DatagramFace(FaceUri(multicastGroup), localUri, std::move(recvSocket))
  , m_multicastGroup(multicastGroup)
  , m_sendSocket(std::move(sendSocket))
{
}

const MulticastUdpFace::protocol::endpoint&
MulticastUdpFace::getMulticastGroup() const
{
  return m_multicastGroup;
}

void
MulticastUdpFace::sendInterest(const Interest& interest)
{
  NFD_LOG_FACE_TRACE(__func__);
  this->emitSignal(onSendInterest, interest);
  sendBlock(interest.wireEncode());
}

void
MulticastUdpFace::sendData(const Data& data)
{
  NFD_LOG_FACE_TRACE(__func__);
  /// \todo After this face implements duplicate suppression, onSendData should
  ///       be emitted only when data is actually sent out. See also #2555
  this->emitSignal(onSendData, data);
  sendBlock(data.wireEncode());
}

void
MulticastUdpFace::sendBlock(const Block& block)
{
  m_sendSocket.async_send_to(boost::asio::buffer(block.wire(), block.size()), m_multicastGroup,
                             bind(&MulticastUdpFace::handleSend, this,
                                  boost::asio::placeholders::error,
                                  boost::asio::placeholders::bytes_transferred,
                                  block));
}

} // namespace nfd
