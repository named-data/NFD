/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#include "face-common.hpp"
#include "common/counter.hpp"

namespace nfd::face {

/**
 * \brief Indicates the state of a transport.
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

/** \brief Counters provided by a transport.
 *  \note The type name TransportCounters is an implementation detail.
 *        Use Transport::Counters in public API.
 */
class TransportCounters
{
public:
  /** \brief Count of incoming packets.
   *
   *  A 'packet' typically means a top-level TLV block.
   *  For a datagram-based transport, an incoming packet that cannot be parsed as TLV
   *  would not be counted.
   */
  PacketCounter nInPackets;

  /** \brief Count of outgoing packets.
   *
   *  A 'packet' typically means a top-level TLV block.
   *  This counter is incremented only if transport is UP.
   */
  PacketCounter nOutPackets;

  /** \brief Total incoming bytes.
   *
   *  This counter includes headers imposed by NFD (such as NDNLP),
   *  but excludes overhead of underlying protocol (such as IP header).
   *  For a datagram-based transport, an incoming packet that cannot be parsed as TLV
   *  would not be counted.
   */
  ByteCounter nInBytes;

  /** \brief Total outgoing bytes.
   *
   *  This counter includes headers imposed by NFD (such as NDNLP),
   *  but excludes overhead of underlying protocol (such as IP header).
   *  This counter is increased only if transport is UP.
   */
  ByteCounter nOutBytes;
};

/**
 * \brief Indicates that the transport has no limit on payload size.
 */
inline constexpr ssize_t MTU_UNLIMITED = -1;

/**
 * \brief (for internal use) Indicates that the MTU field is unset.
 */
inline constexpr ssize_t MTU_INVALID = -2;

/**
 * \brief Indicates that the transport does not support reading the queue capacity/length.
 */
inline constexpr ssize_t QUEUE_UNSUPPORTED = -1;

/**
 * \brief Indicates that the transport was unable to retrieve the queue capacity/length.
 */
inline constexpr ssize_t QUEUE_ERROR = -2;

/**
 * \brief The lower half of a Face.
 * \sa Face, LinkService
 */
class Transport : protected virtual TransportCounters, noncopyable
{
public:
  /** \brief Counters provided by a transport.
   *  \sa TransportCounters
   */
  using Counters = TransportCounters;

  /** \brief Default constructor.
   *
   *  This constructor initializes static properties to invalid values.
   *  Subclass constructor must explicitly set every static property.
   *
   *  This constructor initializes TransportState to UP;
   *  subclass constructor can rely on this default value.
   */
  Transport();

  virtual
  ~Transport();

public:
  /**
   * \brief Set Face and LinkService for this transport.
   * \pre setFaceAndLinkService() has not been called.
   */
  void
  setFaceAndLinkService(Face& face, LinkService& service) noexcept;

  /**
   * \brief Returns the Face to which this transport is attached.
   */
  const Face*
  getFace() const noexcept
  {
    return m_face;
  }

  /**
   * \brief Returns the LinkService to which this transport is attached.
   */
  const LinkService*
  getLinkService() const noexcept
  {
    return m_service;
  }

  /**
   * \brief Returns the LinkService to which this transport is attached.
   */
  LinkService*
  getLinkService() noexcept
  {
    return m_service;
  }

  virtual const Counters&
  getCounters() const
  {
    return *this;
  }

public: // upper interface
  /** \brief Request the transport to be closed.
   *
   *  This operation is effective only if transport is in UP or DOWN state,
   *  otherwise it has no effect.
   *  The transport changes state to CLOSING, and performs cleanup procedure.
   *  The state will be changed to CLOSED when cleanup is complete, which may
   *  happen synchronously or asynchronously.
   */
  void
  close();

  /** \brief Send a link-layer packet.
   *  \param packet the packet to be sent, must be a valid and well-formed TLV block
   *  \note This operation has no effect if getState() is neither UP nor DOWN
   *  \warning Behavior is undefined if packet size exceeds the MTU limit
   */
  void
  send(const Block& packet);

public: // static properties
  /**
   * \brief Returns a FaceUri representing the local endpoint.
   */
  FaceUri
  getLocalUri() const noexcept
  {
    return m_localUri;
  }

  /**
   * \brief Returns a FaceUri representing the remote endpoint.
   */
  FaceUri
  getRemoteUri() const noexcept
  {
    return m_remoteUri;
  }

  /**
   * \brief Returns whether the transport is local or non-local for scope control purposes.
   */
  ndn::nfd::FaceScope
  getScope() const noexcept
  {
    return m_scope;
  }

  /**
   * \brief Returns the current persistency setting of the transport.
   */
  ndn::nfd::FacePersistency
  getPersistency() const noexcept
  {
    return m_persistency;
  }

  /**
   * \brief Check whether the persistency can be changed to \p newPersistency.
   *
   * This function serves as the external API, and invokes the protected function
   * canChangePersistencyToImpl() to perform further checks if \p newPersistency differs
   * from the current persistency.
   *
   * \return true if the change can be performed, false otherwise
   */
  bool
  canChangePersistencyTo(ndn::nfd::FacePersistency newPersistency) const;

  /**
   * \brief Changes the persistency setting of the transport.
   */
  void
  setPersistency(ndn::nfd::FacePersistency newPersistency);

  /**
   * \brief Returns the link type of the transport.
   */
  ndn::nfd::LinkType
  getLinkType() const noexcept
  {
    return m_linkType;
  }

  /**
   * \brief Returns the maximum payload size.
   * \retval MTU_UNLIMITED The transport has no limit on payload size.
   *
   * This size is the maximum packet size that can be sent or received through this transport.
   *
   * For a datagram-based transport, this is typically the Maximum Transmission Unit (MTU),
   * after the overhead of headers introduced by the transport has been accounted for.
   * For a stream-based transport, this is typically unlimited (MTU_UNLIMITED).
   */
  ssize_t
  getMtu() const noexcept
  {
    return m_mtu;
  }

  /**
   * \brief Returns the capacity of the send queue (in bytes).
   * \retval QUEUE_UNSUPPORTED The transport does not support queue capacity retrieval.
   * \retval QUEUE_ERROR The transport was unable to retrieve the queue capacity.
   */
  ssize_t
  getSendQueueCapacity() const noexcept
  {
    return m_sendQueueCapacity;
  }

public: // dynamic properties
  /**
   * \brief Returns the current transport state.
   */
  TransportState
  getState() const noexcept
  {
    return m_state;
  }

  /**
   * \brief Signals when the transport state changes.
   */
  signal::Signal<Transport, TransportState/*old*/, TransportState/*new*/> afterStateChange;

  /**
   * \brief Returns the expiration time of the transport.
   * \retval time::steady_clock::time_point::max() The transport has an indefinite lifetime.
   */
  time::steady_clock::time_point
  getExpirationTime() const noexcept
  {
    return m_expirationTime;
  }

  /**
   * \brief Returns the current send queue length of the transport (in octets).
   * \retval QUEUE_UNSUPPORTED The transport does not support queue length retrieval.
   * \retval QUEUE_ERROR The transport was unable to retrieve the queue length.
   */
  virtual ssize_t
  getSendQueueLength()
  {
    return QUEUE_UNSUPPORTED;
  }

protected: // upper interface to be invoked by subclass
  /**
   * \brief Pass a received link-layer packet to the upper layer for further processing.
   * \param packet The received packet, must be a valid and well-formed TLV block
   * \param endpoint The source endpoint, optional for unicast transports
   * \warning Behavior is undefined if packet size exceeds the MTU limit.
   */
  void
  receive(const Block& packet, const EndpointId& endpoint = {});

protected: // properties to be set by subclass
  void
  setLocalUri(const FaceUri& uri) noexcept
  {
    m_localUri = uri;
  }

  void
  setRemoteUri(const FaceUri& uri) noexcept
  {
    m_remoteUri = uri;
  }

  void
  setScope(ndn::nfd::FaceScope scope) noexcept
  {
    m_scope = scope;
  }

  void
  setLinkType(ndn::nfd::LinkType linkType) noexcept
  {
    m_linkType = linkType;
  }

  void
  setMtu(ssize_t mtu) noexcept;

  void
  setSendQueueCapacity(ssize_t sendQueueCapacity) noexcept
  {
    m_sendQueueCapacity = sendQueueCapacity;
  }

  /** \brief Set transport state.
   *
   *  Only the following transitions are valid:
   *  UP->DOWN, DOWN->UP, UP/DOWN->CLOSING/FAILED, CLOSING/FAILED->CLOSED
   *
   *  \throw std::runtime_error transition is invalid.
   */
  void
  setState(TransportState newState);

  void
  setExpirationTime(const time::steady_clock::time_point& expirationTime) noexcept
  {
    m_expirationTime = expirationTime;
  }

protected: // to be overridden by subclass
  /** \brief Invoked by canChangePersistencyTo to perform the check.
   *
   *  Base class implementation returns false.
   *
   *  \param newPersistency the new persistency, guaranteed to be different from current persistency
   */
  virtual bool
  canChangePersistencyToImpl(ndn::nfd::FacePersistency newPersistency) const;

  /** \brief Invoked after the persistency has been changed.
   *
   *  The base class implementation does nothing.
   *  When overridden in a subclass, the function should update internal states
   *  after persistency setting has been changed.
   */
  virtual void
  afterChangePersistency(ndn::nfd::FacePersistency oldPersistency);

  /** \brief Performs Transport specific operations to close the transport.
   *
   *  This is invoked once by close() after changing state to CLOSING.
   *  It will not be invoked by Transport class if the transport is already CLOSING or CLOSED.
   *
   *  When the cleanup procedure is complete, this method should change state to CLOSED.
   *  This transition can happen synchronously or asynchronously.
   */
  virtual void
  doClose() = 0;

private: // to be overridden by subclass
  /** \brief Performs Transport specific operations to send a packet.
   *  \param packet the packet to be sent, can be assumed to be valid and well-formed
   *  \pre transport state is either UP or DOWN
   */
  virtual void
  doSend(const Block& packet) = 0;

private:
  Face* m_face = nullptr;
  LinkService* m_service = nullptr;
  FaceUri m_localUri;
  FaceUri m_remoteUri;
  ndn::nfd::FaceScope m_scope = ndn::nfd::FACE_SCOPE_NONE;
  ndn::nfd::FacePersistency m_persistency = ndn::nfd::FACE_PERSISTENCY_NONE;
  ndn::nfd::LinkType m_linkType = ndn::nfd::LINK_TYPE_NONE;
  ssize_t m_mtu = MTU_INVALID;
  ssize_t m_sendQueueCapacity = QUEUE_UNSUPPORTED;
  TransportState m_state = TransportState::UP;
  time::steady_clock::time_point m_expirationTime = time::steady_clock::time_point::max();
};

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<Transport>& flh);

template<typename T>
std::enable_if_t<std::is_base_of_v<Transport, T> && !std::is_same_v<Transport, T>,
                 std::ostream&>
operator<<(std::ostream& os, const FaceLogHelper<T>& flh)
{
  return os << FaceLogHelper<Transport>(flh.obj);
}

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_TRANSPORT_HPP
