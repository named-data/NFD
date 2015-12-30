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

#include "forwarder-status-manager.hpp"
#include "fw/forwarder.hpp"
#include "version.hpp"

namespace nfd {

const time::milliseconds STATUS_SERVER_DEFAULT_FRESHNESS = time::milliseconds(5000);

ForwarderStatusManager::ForwarderStatusManager(Forwarder& forwarder, Dispatcher& dispatcher)
  : m_forwarder(forwarder)
  , m_dispatcher(dispatcher)
  , m_startTimestamp(time::system_clock::now())
{
  m_dispatcher.addStatusDataset("status", ndn::mgmt::makeAcceptAllAuthorization(),
                                bind(&ForwarderStatusManager::listGeneralStatus, this, _1, _2, _3));
}

ndn::nfd::ForwarderStatus
ForwarderStatusManager::collectGeneralStatus()
{
  ndn::nfd::ForwarderStatus status;

  status.setNfdVersion(NFD_VERSION_BUILD_STRING);
  status.setStartTimestamp(m_startTimestamp);
  status.setCurrentTimestamp(time::system_clock::now());

  status.setNNameTreeEntries(m_forwarder.getNameTree().size());
  status.setNFibEntries(m_forwarder.getFib().size());
  status.setNPitEntries(m_forwarder.getPit().size());
  status.setNMeasurementsEntries(m_forwarder.getMeasurements().size());
  status.setNCsEntries(m_forwarder.getCs().size());

  const ForwarderCounters& counters = m_forwarder.getCounters();
  status.setNInInterests(counters.nInInterests)
        .setNOutInterests(counters.nOutInterests)
        .setNInDatas(counters.nInData)
        .setNOutDatas(counters.nOutData)
        .setNInNacks(counters.nInNacks)
        .setNOutNacks(counters.nOutNacks);

  return status;
}

void
ForwarderStatusManager::listGeneralStatus(const Name& topPrefix, const Interest& interest,
                                          ndn::mgmt::StatusDatasetContext& context)
{
  static const PartialName PREFIX_STATUS("status");
  static const PartialName PREFIX_STATUS_GENERAL("status/general");

  PartialName subPrefix = interest.getName().getSubName(topPrefix.size());
  if (subPrefix == PREFIX_STATUS_GENERAL || subPrefix == PREFIX_STATUS) {
    context.setPrefix(Name(topPrefix).append(PREFIX_STATUS_GENERAL));
  }
  else {
    context.reject(ndn::mgmt::ControlResponse().setCode(404));
    return;
  }
  // TODO#3379 register the dataset at status/general, and delete these conditions

  context.setExpiry(STATUS_SERVER_DEFAULT_FRESHNESS);

  auto status = this->collectGeneralStatus();
  status.wireEncode().parse();
  for (const auto& subblock : status.wireEncode().elements()) {
    context.append(subblock);
  }
  context.end();
}

} // namespace nfd
