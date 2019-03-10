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

#include "dummy-face.hpp"
#include "dummy-link-service.hpp"
#include "dummy-transport.hpp"

namespace nfd {
namespace face {
namespace tests {

DummyFace::DummyFace(const std::string& localUri, const std::string& remoteUri,
                     ndn::nfd::FaceScope scope, ndn::nfd::FacePersistency persistency,
                     ndn::nfd::LinkType linkType)
  : Face(make_unique<DummyLinkService>(),
         make_unique<DummyTransport>(localUri, remoteUri, scope, persistency, linkType))
  , afterSend(getDummyLinkService()->afterSend)
  , sentInterests(getDummyLinkService()->sentInterests)
  , sentData(getDummyLinkService()->sentData)
  , sentNacks(getDummyLinkService()->sentNacks)
{
  getDummyLinkService()->setPacketLogging(LogSentPackets);
}

void
DummyFace::setState(FaceState state)
{
  static_cast<DummyTransport*>(getTransport())->setState(state);
}

void
DummyFace::receiveInterest(const Interest& interest, const EndpointId& endpointId)
{
  getDummyLinkService()->receiveInterest(interest, endpointId);
}

void
DummyFace::receiveData(const Data& data, const EndpointId& endpointId)
{
  getDummyLinkService()->receiveData(data, endpointId);
}

void
DummyFace::receiveNack(const lp::Nack& nack, const EndpointId& endpointId)
{
  getDummyLinkService()->receiveNack(nack, endpointId);
}

DummyLinkService*
DummyFace::getDummyLinkService() const
{
  return static_cast<DummyLinkService*>(getLinkService());
}

} // namespace tests
} // namespace face
} // namespace nfd
