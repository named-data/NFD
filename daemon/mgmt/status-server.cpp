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

#include "status-server.hpp"
#include "fw/forwarder.hpp"
#include "version.hpp"

namespace nfd {

const Name StatusServer::DATASET_PREFIX = "ndn:/localhost/nfd/status";
const time::milliseconds StatusServer::RESPONSE_FRESHNESS = time::milliseconds(5000);

StatusServer::StatusServer(shared_ptr<AppFace> face, Forwarder& forwarder, ndn::KeyChain& keyChain)
  : m_face(face)
  , m_forwarder(forwarder)
  , m_startTimestamp(time::system_clock::now())
  , m_keyChain(keyChain)
{
  m_face->setInterestFilter(DATASET_PREFIX, bind(&StatusServer::onInterest, this, _2));
}

void
StatusServer::onInterest(const Interest& interest) const
{
  Name name(DATASET_PREFIX);
  name.appendVersion();
  name.appendSegment(0);

  shared_ptr<Data> data = make_shared<Data>(name);
  data->setFreshnessPeriod(RESPONSE_FRESHNESS);

  shared_ptr<ndn::nfd::ForwarderStatus> status = this->collectStatus();
  data->setContent(status->wireEncode());

  m_keyChain.sign(*data);
  m_face->put(*data);
}

shared_ptr<ndn::nfd::ForwarderStatus>
StatusServer::collectStatus() const
{
  shared_ptr<ndn::nfd::ForwarderStatus> status = make_shared<ndn::nfd::ForwarderStatus>();

  status->setNfdVersion(NFD_VERSION_BUILD_STRING);
  status->setStartTimestamp(m_startTimestamp);
  status->setCurrentTimestamp(time::system_clock::now());

  status->setNNameTreeEntries(m_forwarder.getNameTree().size());
  status->setNFibEntries(m_forwarder.getFib().size());
  status->setNPitEntries(m_forwarder.getPit().size());
  status->setNMeasurementsEntries(m_forwarder.getMeasurements().size());
  status->setNCsEntries(m_forwarder.getCs().size());

  m_forwarder.getCounters().copyTo(*status);

  return status;
}

} // namespace nfd
