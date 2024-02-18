/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "face-common.hpp"
#include "link-service.hpp"
#include "transport.hpp"

namespace nfd {
namespace face {

class Channel;

/**
 * \brief Indicates the state of a face.
 */
using FaceState = TransportState;

/**
 * \brief Gives access to the counters provided by Face.
 *
 * This type is a facade that exposes common counters of a Face.
 *
 * get<T>() can be used to access extended counters provided by
 * LinkService or Transport of the Face.
 */
class FaceCounters
{
public:
  FaceCounters(const LinkService::Counters& linkServiceCounters,
               const Transport::Counters& transportCounters);

  /**
   * \brief Returns the counters provided by (a subclass of) LinkService.
   * \tparam T The desired counters type
   * \throw std::bad_cast counters type mismatch
   */
  template<typename T>
  std::enable_if_t<std::is_base_of_v<LinkService::Counters, T>, const T&>
  get() const
  {
    return dynamic_cast<const T&>(m_linkServiceCounters);
  }

  /**
   * \brief Returns the counters provided by (a subclass of) Transport.
   * \tparam T The desired counters type
   * \throw std::bad_cast counters type mismatch
   */
  template<typename T>
  std::enable_if_t<std::is_base_of_v<Transport::Counters, T>, const T&>
  get() const
  {
    return dynamic_cast<const T&>(m_transportCounters);
  }

public:
  const PacketCounter& nInInterests;           ///< \copydoc LinkService::Counters::nInInterests
  const PacketCounter& nOutInterests;          ///< \copydoc LinkService::Counters::nOutInterests
  const PacketCounter& nInterestsExceededRetx; ///< \copydoc LinkService::Counters::nInterestsExceededRetx
  const PacketCounter& nInData;                ///< \copydoc LinkService::Counters::nInData
  const PacketCounter& nOutData;               ///< \copydoc LinkService::Counters::nOutData
  const PacketCounter& nInNacks;               ///< \copydoc LinkService::Counters::nInNacks
  const PacketCounter& nOutNacks;              ///< \copydoc LinkService::Counters::nOutNacks

  const PacketCounter& nInPackets;  ///< \copydoc Transport::Counters::nInPackets
  const PacketCounter& nOutPackets; ///< \copydoc Transport::Counters::nOutPackets
  const ByteCounter& nInBytes;      ///< \copydoc Transport::Counters::nInBytes
  const ByteCounter& nOutBytes;     ///< \copydoc Transport::Counters::nOutBytes

  /// Count of incoming Interests dropped due to HopLimit == 0.
  PacketCounter nInHopLimitZero;
  /// Count of outgoing Interests dropped due to HopLimit == 0 on non-local faces.
  PacketCounter nOutHopLimitZero;

private:
  const LinkService::Counters& m_linkServiceCounters;
  const Transport::Counters& m_transportCounters;
};

/**
 * \brief Generalization of a network interface.
 *
 * A face generalizes a network interface.
 * It provides best-effort network-layer packet delivery services
 * on a physical interface, an overlay tunnel, or a link to a local application.
 *
 * A face combines two parts: LinkService and Transport.
 * Transport is the lower part, which provides best-effort TLV block deliveries.
 * LinkService is the upper part, which translates between network-layer packets
 * and TLV blocks, and may provide additional services such as fragmentation and reassembly.
 */
class Face NFD_FINAL_UNLESS_WITH_TESTS : public std::enable_shared_from_this<Face>, noncopyable
{
public:
  Face(unique_ptr<LinkService> service, unique_ptr<Transport> transport);

  LinkService*
  getLinkService() const noexcept
  {
    return m_service.get();
  }

  Transport*
  getTransport() const noexcept
  {
    return m_transport.get();
  }

  /**
   * \brief Request that the face be closed.
   *
   * This operation is effective only if face is in the UP or DOWN state; otherwise,
   * it has no effect. The face will change state to CLOSING, and then perform a
   * cleanup procedure. When the cleanup is complete, the state will be changed to
   * CLOSED, which may happen synchronously or asynchronously.
   *
   * \warning The face must not be deallocated until its state changes to CLOSED.
   */
  void
  close()
  {
    m_transport->close();
  }

public: // upper interface connected to forwarding
  /**
   * \brief Send Interest.
   */
  void
  sendInterest(const Interest& interest)
  {
    m_service->sendInterest(interest);
  }

  /**
   * \brief Send Data.
   */
  void
  sendData(const Data& data)
  {
    m_service->sendData(data);
  }

  /**
   * \brief Send Nack.
   */
  void
  sendNack(const lp::Nack& nack)
  {
    m_service->sendNack(nack);
  }

  /// \copydoc LinkService::afterReceiveInterest
  signal::Signal<LinkService, Interest, EndpointId>& afterReceiveInterest;

  /// \copydoc LinkService::afterReceiveData
  signal::Signal<LinkService, Data, EndpointId>& afterReceiveData;

  /// \copydoc LinkService::afterReceiveNack
  signal::Signal<LinkService, lp::Nack, EndpointId>& afterReceiveNack;

  /// \copydoc LinkService::onDroppedInterest
  signal::Signal<LinkService, Interest>& onDroppedInterest;

public: // properties
  /**
   * \brief Returns the face ID.
   */
  FaceId
  getId() const noexcept
  {
    return m_id;
  }

  /**
   * \brief Sets the face ID.
   * \note Normally, this should only be invoked by FaceTable.
   */
  void
  setId(FaceId id) noexcept
  {
    m_id = id;
  }

  /**
   * \brief Returns a FaceUri representing the local endpoint.
   */
  FaceUri
  getLocalUri() const noexcept
  {
    return m_transport->getLocalUri();
  }

  /**
   * \brief Returns a FaceUri representing the remote endpoint.
   */
  FaceUri
  getRemoteUri() const noexcept
  {
    return m_transport->getRemoteUri();
  }

  /**
   * \brief Returns whether the face is local or non-local for scope control purposes.
   */
  ndn::nfd::FaceScope
  getScope() const noexcept
  {
    return m_transport->getScope();
  }

  /**
   * \brief Returns the current persistency setting of the face.
   */
  ndn::nfd::FacePersistency
  getPersistency() const noexcept
  {
    return m_transport->getPersistency();
  }

  /**
   * \brief Changes the face persistency setting.
   */
  void
  setPersistency(ndn::nfd::FacePersistency persistency)
  {
    return m_transport->setPersistency(persistency);
  }

  /**
   * \brief Returns the link type of the face (point-to-point, multi-access, ...).
   */
  ndn::nfd::LinkType
  getLinkType() const noexcept
  {
    return m_transport->getLinkType();
  }

  /**
   * \brief Returns the effective MTU of the face.
   *
   * This function is a wrapper. The effective MTU of a face is determined by the link service.
   */
  ssize_t
  getMtu() const
  {
    return m_service->getEffectiveMtu();
  }

  /**
   * \brief Returns the face state.
   */
  FaceState
  getState() const noexcept
  {
    return m_transport->getState();
  }

  /// \copydoc Transport::afterStateChange
  signal::Signal<Transport, FaceState /*old*/, FaceState /*new*/>& afterStateChange;

  /**
   * \brief Returns the expiration time of the face.
   * \retval time::steady_clock::time_point::max() The face has an indefinite lifetime.
   */
  time::steady_clock::time_point
  getExpirationTime() const noexcept
  {
    return m_transport->getExpirationTime();
  }

  const FaceCounters&
  getCounters() const noexcept
  {
    return m_counters;
  }

  FaceCounters&
  getCounters() noexcept
  {
    return m_counters;
  }

  /**
   * \brief Get channel on which face was created (unicast) or the associated channel (multicast).
   */
  weak_ptr<Channel>
  getChannel() const noexcept
  {
    return m_channel;
  }

  /**
   * \brief Set channel on which face was created (unicast) or the associated channel (multicast).
   */
  void
  setChannel(weak_ptr<Channel> channel) noexcept
  {
    m_channel = std::move(channel);
  }

private:
  FaceId m_id = INVALID_FACEID;
  unique_ptr<LinkService> m_service;
  unique_ptr<Transport> m_transport;
  FaceCounters m_counters;
  weak_ptr<Channel> m_channel;
};

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<Face>& flh);

} // namespace face

using face::Face;

} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_HPP
