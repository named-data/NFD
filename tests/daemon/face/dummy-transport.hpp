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

#ifndef NFD_TESTS_DAEMON_FACE_DUMMY_TRANSPORT_HPP
#define NFD_TESTS_DAEMON_FACE_DUMMY_TRANSPORT_HPP

#include "core/common.hpp"

#include "face/transport.hpp"

namespace nfd {
namespace face {
namespace tests {

/** \brief dummy Transport used in unit tests
 */
class DummyTransport : public Transport
{
public:
  DummyTransport(const std::string& localUri = "dummy://",
                 const std::string& remoteUri = "dummy://",
                 ndn::nfd::FaceScope scope = ndn::nfd::FACE_SCOPE_NON_LOCAL,
                 ndn::nfd::FacePersistency persistency = ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                 ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_POINT_TO_POINT,
                 ssize_t mtu = MTU_UNLIMITED,
                 ssize_t sendQueueCapacity = QUEUE_UNSUPPORTED)
    : isClosed(false)
    , m_sendQueueLength(0)
  {
    this->setLocalUri(FaceUri(localUri));
    this->setRemoteUri(FaceUri(remoteUri));
    this->setScope(scope);
    this->setPersistency(persistency);
    this->setLinkType(linkType);
    this->setMtu(mtu);
    this->setSendQueueCapacity(sendQueueCapacity);
  }

  void
  setMtu(ssize_t mtu)
  {
    this->Transport::setMtu(mtu);
  }

  void
  setState(FaceState state)
  {
    this->Transport::setState(state);
  }

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
  receivePacket(Packet&& packet)
  {
    this->receive(std::move(packet));
  }

  void
  receivePacket(Block block)
  {
    this->receive(Packet(std::move(block)));
  }

protected:
  bool
  canChangePersistencyToImpl(ndn::nfd::FacePersistency newPersistency) const override
  {
    return true;
  }

private:
  void
  doClose() override
  {
    isClosed = true;
    this->setState(TransportState::CLOSED);
  }

  void
  doSend(Packet&& packet) override
  {
    sentPackets.push_back(std::move(packet));
  }

public:
  bool isClosed;
  std::vector<Packet> sentPackets;

private:
  ssize_t m_sendQueueLength;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_DUMMY_TRANSPORT_HPP
