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

#include "mgmt/forwarder-status-manager.hpp"
#include "core/version.hpp"

#include "nfd-manager-common-fixture.hpp"

namespace nfd {
namespace tests {

class ForwarderStatusManagerFixture : public NfdManagerCommonFixture
{
protected:
  ForwarderStatusManagerFixture()
    : manager(m_forwarder, m_dispatcher)
    , startTime(time::system_clock::now())
  {
    setTopPrefix();
  }

protected:
  ForwarderStatusManager manager;
  time::system_clock::TimePoint startTime;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestForwarderStatusManager, ForwarderStatusManagerFixture)

BOOST_AUTO_TEST_CASE(GeneralStatusDataset)
{
  // cause counters to be non-zero
  this->advanceClocks(time::seconds(3600));
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
  BOOST_CHECK_GE(m_forwarder.getFib().size(), 1);
  BOOST_CHECK_GE(m_forwarder.getPit().size(), 4);
  BOOST_CHECK_GE(m_forwarder.getMeasurements().size(), 3);
  BOOST_CHECK_GE(m_forwarder.getCs().size(), 2);

  // request
  time::system_clock::TimePoint beforeRequest = time::system_clock::now();
  Interest request("/localhost/nfd/status/general");
  request.setMustBeFresh(true).setChildSelector(1);
  this->receiveInterest(request);
  time::system_clock::TimePoint afterRequest = time::system_clock::now();

  // verify
  Block response = this->concatenateResponses(0, m_responses.size());
  ndn::nfd::ForwarderStatus status;
  BOOST_REQUIRE_NO_THROW(status.wireDecode(response));

  BOOST_CHECK_EQUAL(status.getNfdVersion(), NFD_VERSION_BUILD_STRING);
  BOOST_CHECK_EQUAL(time::toUnixTimestamp(status.getStartTimestamp()), time::toUnixTimestamp(startTime));
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
}

BOOST_AUTO_TEST_SUITE_END() // TestForwarderStatusManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
