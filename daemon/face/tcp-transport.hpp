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

#ifndef NFD_DAEMON_FACE_TCP_TRANSPORT_HPP
#define NFD_DAEMON_FACE_TCP_TRANSPORT_HPP

#include "stream-transport.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace face {

/**
 * \brief A Transport that communicates on a connected TCP socket
 *
 * When persistency is set to permanent, whenever the TCP connection is severed, the transport
 * state is set to DOWN, and the connection is retried periodically with exponential backoff
 * until it is reestablished
 */
class TcpTransport FINAL_UNLESS_WITH_TESTS : public StreamTransport<boost::asio::ip::tcp>
{
public:
  TcpTransport(protocol::socket&& socket, ndn::nfd::FacePersistency persistency, ndn::nfd::FaceScope faceScope);

  ssize_t
  getSendQueueLength() final;

protected:
  bool
  canChangePersistencyToImpl(ndn::nfd::FacePersistency newPersistency) const final;

  void
  afterChangePersistency(ndn::nfd::FacePersistency oldPersistency) final;

  void
  doClose() final;

  void
  handleError(const boost::system::error_code& error) final;

PROTECTED_WITH_TESTS_ELSE_PRIVATE:
  VIRTUAL_WITH_TESTS void
  reconnect();

  VIRTUAL_WITH_TESTS void
  handleReconnect(const boost::system::error_code& error);

  VIRTUAL_WITH_TESTS void
  handleReconnectTimeout();

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /** \brief how long to wait before the first reconnection attempt after the TCP connection has been severed
   */
  static time::milliseconds s_initialReconnectWait;

  /** \brief maximum amount of time to wait before a reconnection attempt
   */
  static time::milliseconds s_maxReconnectWait;

  /** \brief multiplier for the exponential backoff of the reconnection timer
   */
  static float s_reconnectWaitMultiplier;

private:
  typename protocol::endpoint m_remoteEndpoint;

  /** \note valid only when persistency is set to permanent
   */
  scheduler::ScopedEventId m_reconnectEvent;

  /** \note valid only when persistency is set to permanent
   */
  time::milliseconds m_nextReconnectWait;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_TCP_TRANSPORT_HPP
