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

#ifndef NFD_DAEMON_FACE_TRANSPORT_HPP
#define NFD_DAEMON_FACE_TRANSPORT_HPP

#include "core/counter.hpp"
#include "face-log.hpp"

#include <ndn-cxx/encoding/nfd-constants.hpp>

namespace nfd {
namespace face {

class Face;
class LinkService;

/** \brief indicates the state of a transport
 */
enum class TransportState {
  NONE,
  UP, ///< the transport is up and can transmit packets
  DOWN, ///< the transport is temporarily down, and is being recovered
  CLOSING, ///< the transport is being closed gracefully, either by the peer or by a call to close()
  FAILED, ///< the transport is being closed due to a failure
  CLOSED ///< the transport is closed, and can be safely deallocated
};

std::ostream&
operator<<(std::ostream& os, TransportState state);

/** \brief counters provided by Transport
 *  \note The type name 'TransportCounters' is implementation detail.
 *        Use 'Transport::Counters' in public API.
 */
class TransportCounters
{
public:
  /** \brief count of incoming packets
   *
   *  A 'packet' typically means a top-level TLV block.
   *  For a datagram-based transport, an incoming packet that cannot be parsed as TLV
   *  would not be counted.
   */
  PacketCounter nInPackets;

  /** \brief count of outgoing packets
   *
   *  A 'packet' typically means a top-level TLV block.
   *  This counter is incremented only if transport is UP.
   */
  PacketCounter nOutPackets;

  /** \brief total incoming bytes
   *
   *  This counter includes headers imposed by NFD (such as NDNLP),
   *  but excludes overhead of underlying protocol (such as IP header).
   *  For a datagram-based transport, an incoming packet that cannot be parsed as TLV
   *  would not be counted.
   */
  ByteCounter nInBytes;

  /** \brief total outgoing bytes
   *
   *  This counter includes headers imposed by NFD (such as NDNLP),
   *  but excludes overhead of underlying protocol (such as IP header).
   *  This counter is increased only if transport is UP.
   */
  ByteCounter nOutBytes;
};

/** \brief indicates the transport has no limit on payload size
 */
const ssize_t MTU_UNLIMITED = -1;

/** \brief (for internal use) indicates MTU field is unset
 */
const ssize_t MTU_INVALID = -2;

/** \brief indicates that the transport does not support reading the queue capacity/length
 */
const ssize_t QUEUE_UNSUPPORTED = -1;

/** \brief indicates that the transport was unable to retrieve the queue capacity/length
 */
const ssize_t QUEUE_ERROR = -2;

/** \brief the lower part of a Face
 *  \sa Face
 */
class Transport : protected virtual TransportCounters, noncopyable
{
public:
  /** \brief identifies an endpoint on the link
   */
  typedef uint64_t EndpointId;

  /** \brief stores a packet along with the remote endpoint
   */
  class Packet
  {
  public:
    Packet() = default;

    explicit
    Packet(Block&& packet);

  public:
    /** \brief the packet as a TLV block
     */
    Block packet;

    /** \brief identifies the remote endpoint
     *
     *  This ID is only meaningful in the context of the same Transport.
     *  Incoming packets from the same remote endpoint have the same EndpointId,
     *  and incoming packets from different remote endpoints have different EndpointIds.
     */
    EndpointId remoteEndpoint;
  };

  /** \brief counters provided by Transport
   */
  typedef TransportCounters Counters;

  /** \brief constructor
   *
   *  Transport constructor initializes static properties to invalid values.
   *  Subclass constructor must explicitly set every static property.
   *
   *  This constructor initializes TransportState to UP;
   *  subclass constructor can rely on this default value.
   */
  Transport();

  virtual
  ~Transport();

public:
  /** \brief set Face and LinkService for Transport
   *  \pre setFaceAndLinkService has not been called
   */
  void
  setFaceAndLinkService(Face& face, LinkService& service);

  /** \return Face to which this Transport is attached
   */
  const Face*
  getFace() const;

  /** \return LinkService to which this Transport is attached
   */
  const LinkService*
  getLinkService() const;

  /** \return LinkService to which this Transport is attached
   */
  LinkService*
  getLinkService();

  virtual const Counters&
  getCounters() const;

public: // upper interface
  /** \brief request the transport to be closed
   *
   *  This operation is effective only if transport is in UP or DOWN state,
   *  otherwise it has no effect.
   *  The transport changes state to CLOSING, and performs cleanup procedure.
   *  The state will be changed to CLOSED when cleanup is complete, which may
   *  happen synchronously or asynchronously.
   */
  void
  close();

  /** \brief send a link-layer packet
   *  \note This operation has no effect if \p getState() is neither UP nor DOWN
   *  \warning undefined behavior if packet size exceeds MTU limit
   */
  void
  send(Packet&& packet);

protected: // upper interface to be invoked by subclass
  /** \brief receive a link-layer packet
   *  \warning undefined behavior if packet size exceeds MTU limit
   */
  void
  receive(Packet&& packet);

public: // static properties
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

  /** \brief check whether the face persistency can be changed to \p newPersistency
   *
   *  This function serves as the external API, and invokes the protected function
   *  canChangePersistencyToImpl to perform further checks if \p newPersistency differs
   *  from the current persistency.
   *
   *  \return true if the change can be performed, false otherwise
   */
  bool
  canChangePersistencyTo(ndn::nfd::FacePersistency newPersistency) const;

  /** \brief changes face persistency setting
   */
  void
  setPersistency(ndn::nfd::FacePersistency newPersistency);

  /** \return whether face is point-to-point or multi-access
   */
  ndn::nfd::LinkType
  getLinkType() const;

  /** \return maximum payload size
   *  \retval MTU_UNLIMITED transport has no limit on payload size
   *
   *  This size is the maximum packet size that can be sent or received through this transport.
   *
   *  For a datagram-based transport, this is typically the Maximum Transmission Unit (MTU),
   *  after the overhead of headers introduced by the transport has been accounted for.
   *  For a stream-based transport, this is typically unlimited (MTU_UNLIMITED).
   */
  ssize_t
  getMtu() const;

  /** \return capacity of the send queue (in bytes)
   *  \retval QUEUE_UNSUPPORTED transport does not support queue capacity retrieval
   *  \retval QUEUE_ERROR transport was unable to retrieve the queue capacity
   */
  ssize_t
  getSendQueueCapacity() const;

public: // dynamic properties
  /** \return transport state
   */
  TransportState
  getState() const;

  /** \brief signals when transport state changes
   */
  signal::Signal<Transport, TransportState/*old*/, TransportState/*new*/> afterStateChange;

  /** \return expiration time of the transport
   *  \retval time::steady_clock::TimePoint::max() the transport has indefinite lifetime
   */
  time::steady_clock::TimePoint
  getExpirationTime() const;

  /** \return current send queue length of the transport (in octets)
   *  \retval QUEUE_UNSUPPORTED transport does not support queue length retrieval
   *  \retval QUEUE_ERROR transport was unable to retrieve the queue length
   */
  virtual ssize_t
  getSendQueueLength();

protected: // properties to be set by subclass
  void
  setLocalUri(const FaceUri& uri);

  void
  setRemoteUri(const FaceUri& uri);

  void
  setScope(ndn::nfd::FaceScope scope);

  void
  setLinkType(ndn::nfd::LinkType linkType);

  void
  setMtu(ssize_t mtu);

  void
  setSendQueueCapacity(ssize_t sendQueueCapacity);

  /** \brief set transport state
   *
   *  Only the following transitions are valid:
   *  UP->DOWN, DOWN->UP, UP/DOWN->CLOSING/FAILED, CLOSING/FAILED->CLOSED
   *
   *  \throw std::runtime_error transition is invalid.
   */
  void
  setState(TransportState newState);

  void
  setExpirationTime(const time::steady_clock::TimePoint& expirationTime);

protected: // to be overridden by subclass
  /** \brief invoked by canChangePersistencyTo to perform the check
   *
   *  Base class implementation returns false.
   *
   *  \param newPersistency the new persistency, guaranteed to be different from current persistency
   */
  virtual bool
  canChangePersistencyToImpl(ndn::nfd::FacePersistency newPersistency) const;

  /** \brief invoked after the persistency has been changed
   *
   *  The base class implementation does nothing.
   *  When overridden in a subclass, the function should update internal states
   *  after persistency setting has been changed.
   */
  virtual void
  afterChangePersistency(ndn::nfd::FacePersistency oldPersistency);

  /** \brief performs Transport specific operations to close the transport
   *
   *  This is invoked once by \p close() after changing state to CLOSING.
   *  It will not be invoked by Transport class if the transport is already CLOSING or CLOSED.
   *
   *  When the cleanup procedure is complete, this method should change state to CLOSED.
   *  This transition can happen synchronously or asynchronously.
   */
  virtual void
  doClose() = 0;

private: // to be overridden by subclass
  /** \brief performs Transport specific operations to send a packet
   *  \param packet the packet, which must be a well-formed TLV block
   *  \pre state is either UP or DOWN
   */
  virtual void
  doSend(Packet&& packet) = 0;

private:
  Face* m_face;
  LinkService* m_service;
  FaceUri m_localUri;
  FaceUri m_remoteUri;
  ndn::nfd::FaceScope m_scope;
  ndn::nfd::FacePersistency m_persistency;
  ndn::nfd::LinkType m_linkType;
  ssize_t m_mtu;
  size_t m_sendQueueCapacity;
  TransportState m_state;
  time::steady_clock::TimePoint m_expirationTime;
};

inline const Face*
Transport::getFace() const
{
  return m_face;
}

inline const LinkService*
Transport::getLinkService() const
{
  return m_service;
}

inline LinkService*
Transport::getLinkService()
{
  return m_service;
}

inline const Transport::Counters&
Transport::getCounters() const
{
  return *this;
}

inline FaceUri
Transport::getLocalUri() const
{
  return m_localUri;
}

inline void
Transport::setLocalUri(const FaceUri& uri)
{
  m_localUri = uri;
}

inline FaceUri
Transport::getRemoteUri() const
{
  return m_remoteUri;
}

inline void
Transport::setRemoteUri(const FaceUri& uri)
{
  m_remoteUri = uri;
}

inline ndn::nfd::FaceScope
Transport::getScope() const
{
  return m_scope;
}

inline void
Transport::setScope(ndn::nfd::FaceScope scope)
{
  m_scope = scope;
}

inline ndn::nfd::FacePersistency
Transport::getPersistency() const
{
  return m_persistency;
}

inline ndn::nfd::LinkType
Transport::getLinkType() const
{
  return m_linkType;
}

inline void
Transport::setLinkType(ndn::nfd::LinkType linkType)
{
  m_linkType = linkType;
}

inline ssize_t
Transport::getMtu() const
{
  return m_mtu;
}

inline void
Transport::setMtu(ssize_t mtu)
{
  BOOST_ASSERT(mtu == MTU_UNLIMITED || mtu > 0);
  m_mtu = mtu;
}

inline ssize_t
Transport::getSendQueueCapacity() const
{
  return m_sendQueueCapacity;
}

inline void
Transport::setSendQueueCapacity(ssize_t sendQueueCapacity)
{
  m_sendQueueCapacity = sendQueueCapacity;
}

inline TransportState
Transport::getState() const
{
  return m_state;
}

inline time::steady_clock::TimePoint
Transport::getExpirationTime() const
{
  return m_expirationTime;
}

inline void
Transport::setExpirationTime(const time::steady_clock::TimePoint& expirationTime)
{
  m_expirationTime = expirationTime;
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<Transport>& flh);

template<typename T>
typename std::enable_if<std::is_base_of<Transport, T>::value &&
                        !std::is_same<Transport, T>::value, std::ostream&>::type
operator<<(std::ostream& os, const FaceLogHelper<T>& flh)
{
  return os << FaceLogHelper<Transport>(flh.obj);
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_TRANSPORT_HPP
