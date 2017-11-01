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

#ifndef NFD_DAEMON_FACE_FACE_HPP
#define NFD_DAEMON_FACE_FACE_HPP

#include "face-counters.hpp"
#include "face-log.hpp"
#include "link-service.hpp"
#include "transport.hpp"

namespace nfd {
namespace face {

/** \brief identifies a face
 */
typedef uint64_t FaceId;

/// indicates an invalid FaceId
const FaceId INVALID_FACEID = 0;
/// identifies the InternalFace used in management
const FaceId FACEID_INTERNAL_FACE = 1;
/// identifies a packet comes from the ContentStore
const FaceId FACEID_CONTENT_STORE = 254;
/// identifies the NullFace that drops every packet
const FaceId FACEID_NULL = 255;
/// upper bound of reserved FaceIds
const FaceId FACEID_RESERVED_MAX = 255;

/** \brief indicates the state of a face
 */
typedef TransportState FaceState;

/** \brief generalization of a network interface
 *
 *  A face generalizes a network interface.
 *  It provides best-effort network-layer packet delivery services
 *  on a physical interface, an overlay tunnel, or a link to a local application.
 *
 *  A face combines two parts: LinkService and Transport.
 *  Transport is the lower part, which provides best-effort TLV block deliveries.
 *  LinkService is the upper part, which translates between network-layer packets
 *  and TLV blocks, and may provide additional services such as fragmentation and reassembly.
 */
class Face
#ifndef WITH_TESTS
final
#endif
  : public enable_shared_from_this<Face>, noncopyable
{
public:
  Face(unique_ptr<LinkService> service, unique_ptr<Transport> transport);

  LinkService*
  getLinkService() const;

  Transport*
  getTransport() const;

public: // upper interface connected to forwarding
  /** \brief sends Interest on Face
   */
  void
  sendInterest(const Interest& interest);

  /** \brief sends Data on Face
   */
  void
  sendData(const Data& data);

  /** \brief sends Nack on Face
   */
  void
  sendNack(const lp::Nack& nack);

  /** \brief signals on Interest received
   */
  signal::Signal<LinkService, Interest>& afterReceiveInterest;

  /** \brief signals on Data received
   */
  signal::Signal<LinkService, Data>& afterReceiveData;

  /** \brief signals on Nack received
   */
  signal::Signal<LinkService, lp::Nack>& afterReceiveNack;

  /** \brief signals on Interest dropped by reliability system for exceeding allowed number of retx
   */
  signal::Signal<LinkService, Interest>& onDroppedInterest;

public: // static properties
  /** \return face ID
   */
  FaceId
  getId() const;

  /** \brief sets face ID
   *  \note Normally, this should only be invoked by FaceTable.
   */
  void
  setId(FaceId id);

  /** \return a FaceUri representing local endpoint
   */
  FaceUri
  getLocalUri() const;

  /** \return a FaceUri representing remote endpoint
   */
  FaceUri
  getRemoteUri() const;

  /** \return whether face is local or non-local for scope control purpose
   */
  ndn::nfd::FaceScope
  getScope() const;

  /** \return face persistency setting
   */
  ndn::nfd::FacePersistency
  getPersistency() const;

  /** \brief changes face persistency setting
   */
  void
  setPersistency(ndn::nfd::FacePersistency persistency);

  /** \return whether face is point-to-point or multi-access
   */
  ndn::nfd::LinkType
  getLinkType() const;

public: // dynamic properties
  /** \return face state
   */
  FaceState
  getState() const;

  /** \brief signals after face state changed
   */
  signal::Signal<Transport, FaceState/*old*/, FaceState/*new*/>& afterStateChange;

  /** \return expiration time of the face
   *  \retval time::steady_clock::TimePoint::max() the face has an indefinite lifetime
   */
  time::steady_clock::TimePoint
  getExpirationTime() const;

  /** \brief request the face to be closed
   *
   *  This operation is effective only if face is in UP or DOWN state,
   *  otherwise it has no effect.
   *  The face changes state to CLOSING, and performs cleanup procedure.
   *  The state will be changed to CLOSED when cleanup is complete, which may
   *  happen synchronously or asynchronously.
   *
   *  \warning the face must not be deallocated until its state changes to CLOSED
   */
  void
  close();

  const FaceCounters&
  getCounters() const;

private:
  FaceId m_id;
  unique_ptr<LinkService> m_service;
  unique_ptr<Transport> m_transport;
  FaceCounters m_counters;
};

inline LinkService*
Face::getLinkService() const
{
  return m_service.get();
}

inline Transport*
Face::getTransport() const
{
  return m_transport.get();
}

inline void
Face::sendInterest(const Interest& interest)
{
  m_service->sendInterest(interest);
}

inline void
Face::sendData(const Data& data)
{
  m_service->sendData(data);
}

inline void
Face::sendNack(const lp::Nack& nack)
{
  m_service->sendNack(nack);
}

inline FaceId
Face::getId() const
{
  return m_id;
}

inline void
Face::setId(FaceId id)
{
  m_id = id;
}

inline FaceUri
Face::getLocalUri() const
{
  return m_transport->getLocalUri();
}

inline FaceUri
Face::getRemoteUri() const
{
  return m_transport->getRemoteUri();
}

inline ndn::nfd::FaceScope
Face::getScope() const
{
  return m_transport->getScope();
}

inline ndn::nfd::FacePersistency
Face::getPersistency() const
{
  return m_transport->getPersistency();
}

inline void
Face::setPersistency(ndn::nfd::FacePersistency persistency)
{
  return m_transport->setPersistency(persistency);
}

inline ndn::nfd::LinkType
Face::getLinkType() const
{
  return m_transport->getLinkType();
}

inline FaceState
Face::getState() const
{
  return m_transport->getState();
}

inline time::steady_clock::TimePoint
Face::getExpirationTime() const
{
  return m_transport->getExpirationTime();
}

inline void
Face::close()
{
  m_transport->close();
}

inline const FaceCounters&
Face::getCounters() const
{
  return m_counters;
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<Face>& flh);

} // namespace face

using face::FaceId;
using face::Face;

} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_HPP
