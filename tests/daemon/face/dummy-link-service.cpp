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

#include "dummy-link-service.hpp"

namespace nfd {
namespace face {
namespace tests {

void
DummyLinkService::doSendInterest(const Interest& interest, const EndpointId&)
{
  if (m_loggingFlags & LogSentInterests)
    sentInterests.push_back(interest);

  afterSend(tlv::Interest);
}

void
DummyLinkService::doSendData(const Data& data, const EndpointId&)
{
  if (m_loggingFlags & LogSentData)
    sentData.push_back(data);

  afterSend(tlv::Data);
}

void
DummyLinkService::doSendNack(const lp::Nack& nack, const EndpointId&)
{
  if (m_loggingFlags & LogSentNacks)
    sentNacks.push_back(nack);

  afterSend(lp::tlv::Nack);
}

void
DummyLinkService::doReceivePacket(const Block& packet, const EndpointId& endpoint)
{
  if (m_loggingFlags & LogReceivedPackets)
    receivedPackets.push_back({packet, endpoint});
}

} // namespace tests
} // namespace face
} // namespace nfd
