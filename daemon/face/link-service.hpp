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

#ifndef NFD_DAEMON_FACE_LINK_SERVICE_HPP
#define NFD_DAEMON_FACE_LINK_SERVICE_HPP

#include "face-common.hpp"
#include "transport.hpp"
#include "common/counter.hpp"

namespace nfd::face {

/** \brief Counters provided by LinkService.
 *  \note The type name LinkServiceCounters is an implementation detail.
 *        Use LinkService::Counters in public API.
 */
class LinkServiceCounters
{
public:
  /** \brief Count of incoming Interests.
   */
  PacketCounter nInInterests;

  /** \brief Count of outgoing Interests.
   */
  PacketCounter nOutInterests;

  /** \brief Count of Interests dropped by reliability system for exceeding allowed number of retx.
   */
  PacketCounter nInterestsExceededRetx;

  /** \brief Count of incoming Data packets.
   */
  PacketCounter nInData;

  /** \brief Count of outgoing Data packets.
   */
  PacketCounter nOutData;

  /** \brief Count of incoming Nacks.
   */
  PacketCounter nInNacks;

  /** \brief Count of outgoing Nacks.
   */
  PacketCounter nOutNacks;
};

/**
 * \brief The upper half of a Face.
 * \sa Face, Transport
 */
class LinkService : protected virtual LinkServiceCounters, noncopyable
{
public:
  /**
   * \brief %Counters provided by LinkService.
   */
  using Counters = LinkServiceCounters;

public:
  virtual
  ~LinkService();

  /**
   * \brief Set Face and Transport for this LinkService.
   * \pre setFaceAndTransport() has not been called.
   */
  void
  setFaceAndTransport(Face& face, Transport& transport) noexcept;

  /**
   * \brief Returns the Face to which this LinkService is attached.
   */
  const Face*
  getFace() const noexcept
  {
    return m_face;
  }

  /**
   * \brief Returns the Transport to which this LinkService is attached.
   */
  const Transport*
  getTransport() const noexcept
  {
    return m_transport;
  }

  /**
   * \brief Returns the Transport to which this LinkService is attached.
   */
  Transport*
  getTransport() noexcept
  {
    return m_transport;
  }

  virtual const Counters&
  getCounters() const
  {
    return *this;
  }

  virtual ssize_t
  getEffectiveMtu() const;

public: // upper interface to be used by forwarding
  /** \brief Send Interest.
   *  \pre setFaceAndTransport() has been called.
   */
  void
  sendInterest(const Interest& interest);

  /** \brief Send Data.
   *  \pre setFaceAndTransport() has been called.
   */
  void
  sendData(const Data& data);

  /** \brief Send Nack.
   *  \pre setFaceAndTransport() has been called.
   */
  void
  sendNack(const ndn::lp::Nack& nack);

  /** \brief Signals on Interest received.
   */
  signal::Signal<LinkService, Interest, EndpointId> afterReceiveInterest;

  /** \brief Signals on Data received.
   */
  signal::Signal<LinkService, Data, EndpointId> afterReceiveData;

  /** \brief Signals on Nack received.
   */
  signal::Signal<LinkService, lp::Nack, EndpointId> afterReceiveNack;

  /** \brief Signals on Interest dropped by reliability system for exceeding allowed number of retx.
   */
  signal::Signal<LinkService, Interest> onDroppedInterest;

public: // lower interface to be invoked by Transport
  /** \brief Performs LinkService specific operations to receive a lower-layer packet.
   */
  void
  receivePacket(const Block& packet, const EndpointId& endpoint);

protected: // upper interface to be invoked in subclass (receive path termination)
  /** \brief Delivers received Interest to forwarding.
   */
  void
  receiveInterest(const Interest& interest, const EndpointId& endpoint);

  /** \brief Delivers received Data to forwarding.
   */
  void
  receiveData(const Data& data, const EndpointId& endpoint);

  /** \brief Delivers received Nack to forwarding.
   */
  void
  receiveNack(const lp::Nack& nack, const EndpointId& endpoint);

protected: // lower interface to be invoked in subclass (send path termination)
  /** \brief Send a lower-layer packet via Transport.
   */
  void
  sendPacket(const Block& packet);

protected:
  void
  notifyDroppedInterest(const Interest& packet);

private: // upper interface to be overridden in subclass (send path entrypoint)
  /** \brief Performs LinkService specific operations to send an Interest.
   */
  virtual void
  doSendInterest(const Interest& interest) = 0;

  /** \brief Performs LinkService specific operations to send a Data.
   */
  virtual void
  doSendData(const Data& data) = 0;

  /** \brief Performs LinkService specific operations to send a Nack.
   */
  virtual void
  doSendNack(const lp::Nack& nack) = 0;

private: // lower interface to be overridden in subclass
  virtual void
  doReceivePacket(const Block& packet, const EndpointId& endpoint) = 0;

private:
  Face* m_face = nullptr;
  Transport* m_transport = nullptr;
};

inline ssize_t
LinkService::getEffectiveMtu() const
{
  return m_transport->getMtu();
}

inline void
LinkService::receivePacket(const Block& packet, const EndpointId& endpoint)
{
  doReceivePacket(packet, endpoint);
}

inline void
LinkService::sendPacket(const Block& packet)
{
  m_transport->send(packet);
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<LinkService>& flh);

template<typename T>
std::enable_if_t<std::is_base_of_v<LinkService, T> && !std::is_same_v<LinkService, T>,
                 std::ostream&>
operator<<(std::ostream& os, const FaceLogHelper<T>& flh)
{
  return os << FaceLogHelper<LinkService>(flh.obj);
}

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_LINK_SERVICE_HPP
