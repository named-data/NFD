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

#include "mgmt/forwarder-status-manager.hpp"
#include "core/version.hpp"

#include "manager-common-fixture.hpp"

namespace nfd {
namespace tests {

class ForwarderStatusManagerFixture : public ManagerCommonFixture
{
protected:
  ForwarderStatusManagerFixture()
    : m_forwarder(m_faceTable)
    , m_manager(m_forwarder, m_dispatcher)
    , m_startTime(time::system_clock::now())
  {
    setTopPrefix();
  }

protected:
  FaceTable m_faceTable;
  Forwarder m_forwarder;
  ForwarderStatusManager m_manager;
  time::system_clock::TimePoint m_startTime;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestForwarderStatusManager, ForwarderStatusManagerFixture)

BOOST_AUTO_TEST_CASE(GeneralStatusDataset)
{
  // cause counters to be non-zero
  this->advanceClocks(1_h);
  m_forwarder.getFib().insert("ndn:/fib1");
  m_forwarder.getPit().insert(*makeInterest("ndn:/pit1"));
  m_forwarder.getPit().insert(*makeInterest("ndn:/pit2"));
  m_forwarder.getPit().insert(*makeInterest("ndn:/pit3"));
  m_forwarder.getPit().insert(*makeInterest("ndn:/pit4"));
  m_forwarder.getMeasurements().get("ndn:/measurements1");
  m_forwarder.getMeasurements().get("ndn:/measurements2");
  m_forwarder.getMeasurements().get("ndn:/measurements3");
  m_forwarder.getCs().insert(*makeData("ndn:/cs1"));
  m_forwarder.getCs().insert(*makeData("ndn:/cs2"));
  shared_ptr<pit::Entry> pitA = m_forwarder.getPit().insert(*makeInterest("ndn:/pitA")).first;
  pitA->isSatisfied = false;
  m_forwarder.onInterestFinalize(pitA);
  shared_ptr<pit::Entry> pitB = m_forwarder.getPit().insert(*makeInterest("ndn:/pitB")).first;
  pitB->isSatisfied = true;
  m_forwarder.onInterestFinalize(pitB);

  BOOST_CHECK_GE(m_forwarder.getFib().size(), 1);
  BOOST_CHECK_GE(m_forwarder.getPit().size(), 4);
  BOOST_CHECK_GE(m_forwarder.getMeasurements().size(), 3);
  BOOST_CHECK_GE(m_forwarder.getCs().size(), 2);
  BOOST_CHECK_EQUAL(m_forwarder.getCounters().nSatisfiedInterests, 1);
  BOOST_CHECK_EQUAL(m_forwarder.getCounters().nUnsatisfiedInterests, 1);

  // request
  auto beforeRequest = time::system_clock::now();
  receiveInterest(Interest("/localhost/nfd/status/general").setCanBePrefix(true));
  auto afterRequest = time::system_clock::now();

  // verify
  Block response = this->concatenateResponses(0, m_responses.size());
  ndn::nfd::ForwarderStatus status;
  BOOST_REQUIRE_NO_THROW(status.wireDecode(response));

  BOOST_CHECK_EQUAL(status.getNfdVersion(), NFD_VERSION_BUILD_STRING);
  BOOST_CHECK_EQUAL(time::toUnixTimestamp(status.getStartTimestamp()), time::toUnixTimestamp(m_startTime));
  BOOST_CHECK_GE(time::toUnixTimestamp(status.getCurrentTimestamp()), time::toUnixTimestamp(beforeRequest));
  BOOST_CHECK_LE(time::toUnixTimestamp(status.getCurrentTimestamp()), time::toUnixTimestamp(afterRequest));

  // Interest/Data toward ForwarderStatusManager don't go through Forwarder,
  // so request and response won't affect table size
  BOOST_CHECK_EQUAL(status.getNNameTreeEntries(), m_forwarder.getNameTree().size());
  BOOST_CHECK_EQUAL(status.getNFibEntries(), m_forwarder.getFib().size());
  BOOST_CHECK_EQUAL(status.getNPitEntries(), m_forwarder.getPit().size());
  BOOST_CHECK_EQUAL(status.getNMeasurementsEntries(), m_forwarder.getMeasurements().size());
  BOOST_CHECK_EQUAL(status.getNCsEntries(), m_forwarder.getCs().size());
  // TODO#3325 check packet counter values

  BOOST_CHECK_EQUAL(status.getNSatisfiedInterests(), m_forwarder.getCounters().nSatisfiedInterests);
  BOOST_CHECK_EQUAL(status.getNUnsatisfiedInterests(), m_forwarder.getCounters().nUnsatisfiedInterests);
}

BOOST_AUTO_TEST_SUITE_END() // TestForwarderStatusManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
