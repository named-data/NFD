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

#include "transport.hpp"
#include "face.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT("Transport");

std::ostream&
operator<<(std::ostream& os, TransportState state)
{
  switch (state) {
  case TransportState::UP:
    return os << "UP";
  case TransportState::DOWN:
    return os << "DOWN";
  case TransportState::CLOSING:
    return os << "CLOSING";
  case TransportState::FAILED:
    return os << "FAILED";
  case TransportState::CLOSED:
    return os << "CLOSED";
  default:
    return os << "NONE";
  }
}

Transport::Packet::Packet(Block&& packet1)
  : packet(std::move(packet1))
  , remoteEndpoint(0)
{
}

Transport::Transport()
  : m_face(nullptr)
  , m_service(nullptr)
  , m_scope(ndn::nfd::FACE_SCOPE_NONE)
  , m_persistency(ndn::nfd::FACE_PERSISTENCY_NONE)
  , m_linkType(ndn::nfd::LINK_TYPE_NONE)
  , m_mtu(MTU_INVALID)
  , m_state(TransportState::UP)
  , m_expirationTime(time::steady_clock::TimePoint::max())
{
}

Transport::~Transport()
{
}

void
Transport::setFaceAndLinkService(Face& face, LinkService& service)
{
  BOOST_ASSERT(m_face == nullptr);
  BOOST_ASSERT(m_service == nullptr);

  m_face = &face;
  m_service = &service;
}

void
Transport::close()
{
  if (m_state != TransportState::UP && m_state != TransportState::DOWN) {
    return;
  }

  this->setState(TransportState::CLOSING);
  this->doClose();
  // warning: don't access any fields after this:
  // the Transport may be deallocated if doClose changes state to CLOSED
}

void
Transport::send(Packet&& packet)
{
  BOOST_ASSERT(this->getMtu() == MTU_UNLIMITED ||
               packet.packet.size() <= static_cast<size_t>(this->getMtu()));

  TransportState state = this->getState();
  if (state != TransportState::UP && state != TransportState::DOWN) {
    NFD_LOG_FACE_TRACE("send ignored in " << state << " state");
    return;
  }

  if (state == TransportState::UP) {
    ++this->nOutPackets;
    this->nOutBytes += packet.packet.size();
  }

  this->doSend(std::move(packet));
}

void
Transport::receive(Packet&& packet)
{
  BOOST_ASSERT(this->getMtu() == MTU_UNLIMITED ||
               packet.packet.size() <= static_cast<size_t>(this->getMtu()));

  ++this->nInPackets;
  this->nInBytes += packet.packet.size();

  m_service->receivePacket(std::move(packet));
}

void
Transport::setPersistency(ndn::nfd::FacePersistency newPersistency)
{
  if (m_persistency == newPersistency) {
    return;
  }

  if (newPersistency == ndn::nfd::FACE_PERSISTENCY_NONE) {
    throw std::runtime_error("invalid persistency transition");
  }

  if (m_persistency != ndn::nfd::FACE_PERSISTENCY_NONE) {
    this->beforeChangePersistency(newPersistency);
    NFD_LOG_FACE_DEBUG("setPersistency " << m_persistency << " -> " << newPersistency);
  }

  m_persistency = newPersistency;
}

void
Transport::setState(TransportState newState)
{
  if (m_state == newState) {
    return;
  }

  bool isValid = false;
  switch (m_state) {
    case TransportState::UP:
      isValid = newState == TransportState::DOWN ||
                newState == TransportState::CLOSING ||
                newState == TransportState::FAILED;
      break;
    case TransportState::DOWN:
      isValid = newState == TransportState::UP ||
                newState == TransportState::CLOSING ||
                newState == TransportState::FAILED;
      break;
    case TransportState::CLOSING:
    case TransportState::FAILED:
      isValid = newState == TransportState::CLOSED;
      break;
    default:
      break;
  }

  if (!isValid) {
    throw std::runtime_error("invalid state transition");
  }

  NFD_LOG_FACE_INFO("setState " << m_state << " -> " << newState);

  TransportState oldState = m_state;
  m_state = newState;
  afterStateChange(oldState, newState);
  // warning: don't access any fields after this:
  // the Transport may be deallocated in the signal handler if newState is CLOSED
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<Transport>& flh)
{
  const Transport& transport = flh.obj;
  const Face* face = transport.getFace();
  FaceId faceId = face == nullptr ? INVALID_FACEID : face->getId();

  os << "[id=" << faceId << ",local=" << transport.getLocalUri()
     << ",remote=" << transport.getRemoteUri() << "] ";
  return os;
}

} // namespace face
} // namespace nfd
