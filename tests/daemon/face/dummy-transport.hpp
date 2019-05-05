/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_FACE_DUMMY_TRANSPORT_HPP
#define NFD_TESTS_DAEMON_FACE_DUMMY_TRANSPORT_HPP

#include "core/common.hpp"

#include "face/null-transport.hpp"

namespace nfd {
namespace face {
namespace tests {

struct TxPacket
{
  Block packet;
  EndpointId endpoint;
};

/** \brief Dummy Transport type used in unit tests.
 *
 *  All packets sent through this transport are stored in `sentPackets`.
 *  Reception of a packet can be simulated by invoking `receivePacket()`.
 *  All persistency changes are recorded in `persistencyHistory`.
 */
template<bool CAN_CHANGE_PERSISTENCY>
class DummyTransportBase : public NullTransport
{
public:
  explicit
  DummyTransportBase(const std::string& localUri = "dummy://",
                     const std::string& remoteUri = "dummy://",
                     ndn::nfd::FaceScope scope = ndn::nfd::FACE_SCOPE_NON_LOCAL,
                     ndn::nfd::FacePersistency persistency = ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_POINT_TO_POINT,
                     ssize_t mtu = MTU_UNLIMITED,
                     ssize_t sendQueueCapacity = QUEUE_UNSUPPORTED)
    : NullTransport(FaceUri(localUri), FaceUri(remoteUri), scope, persistency)
  {
    this->setLinkType(linkType);
    this->setMtu(mtu);
    this->setSendQueueCapacity(sendQueueCapacity);
  }

  using NullTransport::setMtu;
  using NullTransport::setState;

  ssize_t
  getSendQueueLength() override
  {
    return m_sendQueueLength;
  }

  void
  setSendQueueLength(ssize_t sendQueueLength)
  {
    m_sendQueueLength = sendQueueLength;
  }

  void
  receivePacket(const Block& block)
  {
    receive(block);
  }

protected:
  bool
  canChangePersistencyToImpl(ndn::nfd::FacePersistency) const override
  {
    return CAN_CHANGE_PERSISTENCY;
  }

  void
  afterChangePersistency(ndn::nfd::FacePersistency old) override
  {
    persistencyHistory.push_back(old);
  }

private:
  void
  doSend(const Block& packet, const EndpointId& endpoint) override
  {
    sentPackets.push_back({packet, endpoint});
  }

public:
  std::vector<ndn::nfd::FacePersistency> persistencyHistory;
  std::vector<TxPacket> sentPackets;

private:
  ssize_t m_sendQueueLength = 0;
};

using DummyTransport = DummyTransportBase<true>;

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_DUMMY_TRANSPORT_HPP
