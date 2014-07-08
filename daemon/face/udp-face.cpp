/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#include "udp-face.hpp"

#ifdef __linux__
#include <netinet/in.h> // for IP_MTU_DISCOVER and IP_PMTUDISC_DONT
#include <sys/socket.h> // for setsockopt()
#endif

namespace nfd {

NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(DatagramFace, UdpFace::protocol, "UdpFace");

UdpFace::UdpFace(const shared_ptr<UdpFace::protocol::socket>& socket,
                 bool isOnDemand,
                 const time::seconds& idleTimeout)
  : DatagramFace<protocol>(FaceUri(socket->remote_endpoint()),
                           FaceUri(socket->local_endpoint()),
                           socket, isOnDemand)
  , m_idleTimeout(idleTimeout)
  , m_lastIdleCheck(time::steady_clock::now())
{
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
  if (::setsockopt(socket->native_handle(), IPPROTO_IP,
                   IP_MTU_DISCOVER, &value, sizeof(value)) < 0)
    {
      NFD_LOG_WARN("[id:" << this->getId()
                   << ",uri:" << this->getRemoteUri()
                   << "] Failed to disable path MTU discovery");
    }
#endif

  if (isOnDemand && m_idleTimeout > time::seconds::zero()) {
    m_closeIfIdleEvent = scheduler::schedule(m_idleTimeout,
                                             bind(&UdpFace::closeIfIdle, this));
  }
}

UdpFace::~UdpFace()
{
  scheduler::cancel(m_closeIfIdleEvent);
}

ndn::nfd::FaceStatus
UdpFace::getFaceStatus() const
{
  ndn::nfd::FaceStatus status = Face::getFaceStatus();
  if (isOnDemand()) {
    time::milliseconds left = m_idleTimeout - time::duration_cast<time::milliseconds>(
      time::steady_clock::now() - m_lastIdleCheck);
    if (left < time::milliseconds::zero())
      left = time::milliseconds::zero();

    if (hasBeenUsedRecently()) {
      status.setExpirationPeriod(left + m_idleTimeout);
    }
    else {
      status.setExpirationPeriod(left);
    }
  }
  return status;
}

void
UdpFace::closeIfIdle()
{
  // Face can be switched from on-demand to non-on-demand mode
  // (non-on-demand -> on-demand transition is not allowed)
  if (isOnDemand()) {
    if (!hasBeenUsedRecently()) {
      // face has been idle since the last time closeIfIdle
      // has been called. Going to close it
      NFD_LOG_DEBUG("Found idle face id: " << getId());

      NFD_LOG_INFO("[id:" << this->getId()
                   << ",uri:" << this->getRemoteUri()
                   << "] Idle for more than " << m_idleTimeout << ", closing");
      close();
    }
    else {
      resetRecentUsage();

      m_lastIdleCheck = time::steady_clock::now();
      m_closeIfIdleEvent = scheduler::schedule(m_idleTimeout,
                                               bind(&UdpFace::closeIfIdle, this));
    }
  }
  // else do nothing and do not reschedule the event
}

} // namespace nfd
