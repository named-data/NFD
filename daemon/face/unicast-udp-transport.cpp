/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#include "unicast-udp-transport.hpp"
#include "udp-protocol.hpp"

#ifdef __linux__
#include <cerrno>       // for errno
#include <cstring>      // for std::strerror()
#include <netinet/in.h> // for IP_MTU_DISCOVER and IP_PMTUDISC_DONT
#include <sys/socket.h> // for setsockopt()
#endif

namespace nfd {
namespace face {

NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(DatagramTransport, UnicastUdpTransport::protocol,
                                               "UnicastUdpTransport");

UnicastUdpTransport::UnicastUdpTransport(protocol::socket&& socket,
                                         ndn::nfd::FacePersistency persistency,
                                         time::nanoseconds idleTimeout)
  : DatagramTransport(std::move(socket))
  , m_idleTimeout(idleTimeout)
{
  this->setLocalUri(FaceUri(m_socket.local_endpoint()));
  this->setRemoteUri(FaceUri(m_socket.remote_endpoint()));
  this->setScope(ndn::nfd::FACE_SCOPE_NON_LOCAL);
  this->setPersistency(persistency);
  this->setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  this->setMtu(udp::computeMtu(m_socket.local_endpoint()));

  NFD_LOG_FACE_INFO("Creating transport");

#ifdef __linux__
  //
  // By default, Linux does path MTU discovery on IPv4 sockets,
  // and sets the DF (Don't Fragment) flag on datagrams smaller
  // than the interface MTU. However this does not work for us,
  // because we cannot properly respond to ICMP "packet too big"
  // messages by fragmenting the packet at the application level,
  // since we want to rely on IP for fragmentation and reassembly.
  //
  // Therefore, we disable PMTU discovery, which prevents the kernel
  // from setting the DF flag on outgoing datagrams, and thus allows
  // routers along the path to perform fragmentation as needed.
  //
  const int value = IP_PMTUDISC_DONT;
  if (::setsockopt(m_socket.native_handle(), IPPROTO_IP,
                   IP_MTU_DISCOVER, &value, sizeof(value)) < 0) {
    NFD_LOG_FACE_WARN("Failed to disable path MTU discovery: " << std::strerror(errno));
  }
#endif

  if (getPersistency() == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND &&
      m_idleTimeout > time::nanoseconds::zero()) {
    scheduleClosureWhenIdle();
  }
}

bool
UnicastUdpTransport::canChangePersistencyToImpl(ndn::nfd::FacePersistency newPersistency) const
{
  return true;
}

void
UnicastUdpTransport::afterChangePersistency(ndn::nfd::FacePersistency oldPersistency)
{
  if (getPersistency() == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND &&
      m_idleTimeout > time::nanoseconds::zero()) {
    scheduleClosureWhenIdle();
  }
  else {
    m_closeIfIdleEvent.cancel();
    setExpirationTime(time::steady_clock::TimePoint::max());
  }
}

void
UnicastUdpTransport::scheduleClosureWhenIdle()
{
  m_closeIfIdleEvent = scheduler::schedule(m_idleTimeout, [this] {
    if (!hasRecentlyReceived()) {
      NFD_LOG_FACE_INFO("Closing due to inactivity");
      this->close();
    }
    else {
      resetRecentlyReceived();
      scheduleClosureWhenIdle();
    }
  });
  setExpirationTime(time::steady_clock::now() + m_idleTimeout);
}

} // namespace face
} // namespace nfd
