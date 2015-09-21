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

#ifndef NFD_DAEMON_FACE_LINK_SERVICE_HPP
#define NFD_DAEMON_FACE_LINK_SERVICE_HPP

#include "transport.hpp"
#include "face-log.hpp"

namespace nfd {
namespace face {

class LpFace;

/** \brief the upper part of an LpFace
 *  \sa LpFace
 */
class LinkService : noncopyable
{
public:
  LinkService();

  virtual
  ~LinkService();

  /** \brief set Face and Transport for LinkService
   *  \pre setFaceAndTransport has not been called
   */
  void
  setFaceAndTransport(LpFace& face, Transport& transport);

  /** \return Face to which this LinkService is attached
   */
  const LpFace*
  getFace() const;

  /** \return Transport to which this LinkService is attached
   */
  const Transport*
  getTransport() const;

  /** \return Transport to which this LinkService is attached
   */
  Transport*
  getTransport();

public: // upper interface to be used by forwarding
  /** \brief send Interest
   *  \pre setTransport has been called
   */
  void
  sendInterest(const Interest& interest);

  /** \brief send Data
   *  \pre setTransport has been called
   */
  void
  sendData(const Data& data);

  /** \brief send Nack
   *  \pre setTransport has been called
   */
  void
  sendNack(const ndn::lp::Nack& nack);

  /** \brief signals on Interest received
   */
  signal::Signal<LinkService, Interest> afterReceiveInterest;

  /** \brief signals on Data received
   */
  signal::Signal<LinkService, Data> afterReceiveData;

  /** \brief signals on Nack received
   */
  signal::Signal<LinkService, lp::Nack> afterReceiveNack;

private: // upper interface to be overridden in subclass (send path entrypoint)
  /** \brief performs LinkService specific operations to send an Interest
   */
  virtual void
  doSendInterest(const Interest& interest) = 0;

  /** \brief performs LinkService specific operations to send a Data
   */
  virtual void
  doSendData(const Data& data) = 0;

  /** \brief performs LinkService specific operations to send a Nack
   */
  virtual void
  doSendNack(const lp::Nack& nack) = 0;

protected: // upper interface to be invoked in subclass (receive path termination)
  /** \brief delivers received Interest to forwarding
   */
  void
  receiveInterest(const Interest& interest);

  /** \brief delivers received Data to forwarding
   */
  void
  receiveData(const Data& data);

  /** \brief delivers received Nack to forwarding
   */
  void
  receiveNack(const lp::Nack& nack);

public: // lower interface to be invoked by Transport
  /** \brief performs LinkService specific operations to receive a lower-layer packet
   */
  void
  receivePacket(Transport::Packet&& packet);

protected: // lower interface to be invoked in subclass (send path termination)
  /** \brief sends a lower-layer packet via Transport
   */
  void
  sendPacket(Transport::Packet&& packet);

private: // lower interface to be overridden in subclass
  virtual void
  doReceivePacket(Transport::Packet&& packet) = 0;

private:
  LpFace* m_face;
  Transport* m_transport;
  NetworkLayerCounters* m_counters; // TODO#3177 change into NetCounters
};

inline const LpFace*
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

inline void
LinkService::receivePacket(Transport::Packet&& packet)
{
  doReceivePacket(std::move(packet));
}

inline void
LinkService::sendPacket(Transport::Packet&& packet)
{
  m_transport->send(std::move(packet));
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
