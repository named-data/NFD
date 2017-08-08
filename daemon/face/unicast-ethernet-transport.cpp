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

#include "unicast-ethernet-transport.hpp"

#include <stdio.h>  // for snprintf()

namespace nfd {
namespace face {

NFD_LOG_INIT("UnicastEthernetTransport");

UnicastEthernetTransport::UnicastEthernetTransport(const ndn::net::NetworkInterface& localEndpoint,
                                                   const ethernet::Address& remoteEndpoint,
                                                   ndn::nfd::FacePersistency persistency,
                                                   time::nanoseconds idleTimeout)
  : EthernetTransport(localEndpoint, remoteEndpoint)
  , m_idleTimeout(idleTimeout)
{
  this->setLocalUri(FaceUri::fromDev(m_interfaceName));
  this->setRemoteUri(FaceUri(m_destAddress));
  this->setScope(ndn::nfd::FACE_SCOPE_NON_LOCAL);
  this->setPersistency(persistency);
  this->setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  this->setMtu(localEndpoint.getMtu());

  NFD_LOG_FACE_INFO("Creating transport");

  char filter[110];
  // note #1: we cannot use std::snprintf because it's not available
  //          on some platforms (see #2299)
  // note #2: "not vlan" must appear last in the filter expression, or the
  //          rest of the filter won't work as intended (see pcap-filter(7))
  snprintf(filter, sizeof(filter),
           "(ether proto 0x%x) && (ether src %s) && (ether dst %s) && (not vlan)",
           ethernet::ETHERTYPE_NDN,
           m_destAddress.toString().data(),
           m_srcAddress.toString().data());
  m_pcap.setPacketFilter(filter);

  if (getPersistency() == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND &&
      m_idleTimeout > time::nanoseconds::zero()) {
    scheduleClosureWhenIdle();
  }
}

bool
UnicastEthernetTransport::canChangePersistencyToImpl(ndn::nfd::FacePersistency newPersistency) const
{
  return true;
}

void
UnicastEthernetTransport::afterChangePersistency(ndn::nfd::FacePersistency oldPersistency)
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
UnicastEthernetTransport::scheduleClosureWhenIdle()
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
