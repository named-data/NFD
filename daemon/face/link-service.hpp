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

#ifndef NFD_DAEMON_FACE_LINK_SERVICE_HPP
#define NFD_DAEMON_FACE_LINK_SERVICE_HPP

#include "face-common.hpp"
#include "transport.hpp"
#include "common/counter.hpp"

namespace nfd {
namespace face {

/** \brief counters provided by LinkService
 *  \note The type name 'LinkServiceCounters' is implementation detail.
 *        Use 'LinkService::Counters' in public API.
 */
class LinkServiceCounters
{
public:
  /** \brief count of incoming Interests
   */
  PacketCounter nInInterests;

  /** \brief count of outgoing Interests
   */
  PacketCounter nOutInterests;

  /** \brief count of Interests dropped by reliability system for exceeding allowed number of retx
   */
  PacketCounter nDroppedInterests;

  /** \brief count of incoming Data packets
   */
  PacketCounter nInData;

  /** \brief count of outgoing Data packets
   */
  PacketCounter nOutData;

  /** \brief count of incoming Nacks
   */
  PacketCounter nInNacks;

  /** \brief count of outgoing Nacks
   */
  PacketCounter nOutNacks;
};

/** \brief the upper part of a Face
 *  \sa Face
 */
class LinkService : protected virtual LinkServiceCounters, noncopyable
{
public:
  /** \brief counters provided by LinkService
   */
  typedef LinkServiceCounters Counters;

public:
  LinkService();

  virtual
  ~LinkService();

  /** \brief set Face and Transport for LinkService
   *  \pre setFaceAndTransport has not been called
   */
  void
  setFaceAndTransport(Face& face, Transport& transport);

  /** \return Face to which this LinkService is attached
   */
  const Face*
  getFace() const;

  /** \return Transport to which this LinkService is attached
   */
  const Transport*
  getTransport() const;

  /** \return Transport to which this LinkService is attached
   */
  Transport*
  getTransport();

  virtual const Counters&
  getCounters() const;

public: // upper interface to be used by forwarding
  /** \brief Send Interest to \p endpoint
   *  \pre setTransport has been called
   */
  void
  sendInterest(const Interest& interest, const EndpointId& endpoint);

  /** \brief Send Data to \p endpoint
   *  \pre setTransport has been called
   */
  void
  sendData(const Data& data, const EndpointId& endpoint);

  /** \brief Send Nack to \p endpoint
   *  \pre setTransport has been called
   */
  void
  sendNack(const ndn::lp::Nack& nack, const EndpointId& endpoint);

  /** \brief signals on Interest received
   */
  signal::Signal<LinkService, Interest, EndpointId> afterReceiveInterest;

  /** \brief signals on Data received
   */
  signal::Signal<LinkService, Data, EndpointId> afterReceiveData;

  /** \brief signals on Nack received
   */
  signal::Signal<LinkService, lp::Nack, EndpointId> afterReceiveNack;

  /** \brief signals on Interest dropped by reliability system for exceeding allowed number of retx
   */
  signal::Signal<LinkService, Interest> onDroppedInterest;

public: // lower interface to be invoked by Transport
  /** \brief performs LinkService specific operations to receive a lower-layer packet
   */
  void
  receivePacket(const Block& packet, const EndpointId& endpoint);

protected: // upper interface to be invoked in subclass (receive path termination)
  /** \brief delivers received Interest to forwarding
   */
  void
  receiveInterest(const Interest& interest, const EndpointId& endpoint);

  /** \brief delivers received Data to forwarding
   */
  void
  receiveData(const Data& data, const EndpointId& endpoint);

  /** \brief delivers received Nack to forwarding
   */
  void
  receiveNack(const lp::Nack& nack, const EndpointId& endpoint);

protected: // lower interface to be invoked in subclass (send path termination)
  /** \brief send a lower-layer packet via Transport to \p endpoint
   */
  void
  sendPacket(const Block& packet, const EndpointId& endpoint);

protected:
  void
  notifyDroppedInterest(const Interest& packet);

private: // upper interface to be overridden in subclass (send path entrypoint)
  /** \brief performs LinkService specific operations to send an Interest to \p endpoint
   */
  virtual void
  doSendInterest(const Interest& interest, const EndpointId& endpoint) = 0;

  /** \brief performs LinkService specific operations to send a Data to \p endpoint
   */
  virtual void
  doSendData(const Data& data, const EndpointId& endpoint) = 0;

  /** \brief performs LinkService specific operations to send a Nack to \p endpoint
   */
  virtual void
  doSendNack(const lp::Nack& nack, const EndpointId& endpoint) = 0;

private: // lower interface to be overridden in subclass
  virtual void
  doReceivePacket(const Block& packet, const EndpointId& endpoint) = 0;

private:
  Face* m_face;
  Transport* m_transport;
};

inline const Face*
LinkService::getFace() const
{
  return m_face;
}

inline const Transport*
LinkService::getTransport() const
{
  return m_transport;
}

inline Transport*
LinkService::getTransport()
{
  return m_transport;
}

inline const LinkService::Counters&
LinkService::getCounters() const
{
  return *this;
}

inline void
LinkService::receivePacket(const Block& packet, const EndpointId& endpoint)
{
  doReceivePacket(packet, endpoint);
}

inline void
LinkService::sendPacket(const Block& packet, const EndpointId& endpoint)
{
  m_transport->send(packet, endpoint);
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<LinkService>& flh);

template<typename T>
typename std::enable_if<std::is_base_of<LinkService, T>::value &&
                        !std::is_same<LinkService, T>::value, std::ostream&>::type
operator<<(std::ostream& os, const FaceLogHelper<T>& flh)
{
  return os << FaceLogHelper<LinkService>(flh.obj);
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_LINK_SERVICE_HPP
