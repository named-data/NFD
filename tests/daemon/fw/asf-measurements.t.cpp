/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "fw/asf-measurements.hpp"

#include "tests/daemon/face/dummy-face.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace fw {
namespace asf {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_AUTO_TEST_SUITE(TestAsfMeasurements)

BOOST_AUTO_TEST_SUITE(TestRttStats)

BOOST_FIXTURE_TEST_CASE(Basic, BaseFixture)
{
  RttStats stats;

  BOOST_CHECK_EQUAL(stats.getRtt(), RttStats::RTT_NO_MEASUREMENT);
  BOOST_CHECK_EQUAL(stats.getSrtt(), RttStats::RTT_NO_MEASUREMENT);

  // Receive Data back in 50ms
  RttEstimator::Duration rtt(time::milliseconds(50));
  stats.addRttMeasurement(rtt);

  BOOST_CHECK_EQUAL(stats.getRtt(), rtt);
  BOOST_CHECK_EQUAL(stats.getSrtt(), rtt);

  // Receive Data back in 50ms
  stats.addRttMeasurement(rtt);

  BOOST_CHECK_EQUAL(stats.getRtt(), rtt);
  BOOST_CHECK_EQUAL(stats.getSrtt(), rtt);

  // Receive Data back with a higher RTT
  RttEstimator::Duration higherRtt(time::milliseconds(100));
  stats.addRttMeasurement(higherRtt);

  BOOST_CHECK_EQUAL(stats.getRtt(), higherRtt);
  BOOST_CHECK_GT(stats.getSrtt(), rtt);
  BOOST_CHECK_LT(stats.getSrtt(), higherRtt);

  // Simulate timeout
  RttStats::Rtt previousSrtt = stats.getSrtt();

  stats.recordTimeout();

  BOOST_CHECK_EQUAL(stats.getRtt(), RttStats::RTT_TIMEOUT);
  BOOST_CHECK_EQUAL(stats.getSrtt(), previousSrtt);
}

BOOST_AUTO_TEST_SUITE_END() // TestRttStats

BOOST_AUTO_TEST_SUITE(TestFaceInfo)

BOOST_FIXTURE_TEST_CASE(Basic, UnitTestTimeFixture)
{
  FaceInfo info;

  scheduler::EventId id = scheduler::schedule(time::seconds(1), []{});
  ndn::Name interestName("/ndn/interest");

  // Receive Interest and forward to next hop; should update RTO information
  info.setTimeoutEvent(id, interestName);
  BOOST_CHECK_EQUAL(info.isTimeoutScheduled(), true);

  // If the strategy tries to schedule an RTO when one is already scheduled, throw an exception
  BOOST_CHECK_THROW(info.setTimeoutEvent(id, interestName), FaceInfo::Error);

  // Receive Data
  shared_ptr<Interest> interest = makeInterest(interestName);
  shared_ptr<pit::Entry> pitEntry = make_shared<pit::Entry>(*interest);
  std::shared_ptr<DummyFace> face = make_shared<DummyFace>();

  pitEntry->insertOrUpdateOutRecord(*face, *interest);

  RttEstimator::Duration rtt(50);
  this->advanceClocks(time::milliseconds(5), rtt);

  info.recordRtt(pitEntry, *face);
  info.cancelTimeoutEvent(interestName);

  BOOST_CHECK_EQUAL(info.getRtt(), rtt);
  BOOST_CHECK_EQUAL(info.getSrtt(), rtt);

  // Send out another Interest which times out
  info.setTimeoutEvent(id, interestName);

  info.recordTimeout(interestName);
  BOOST_CHECK_EQUAL(info.getRtt(), RttStats::RTT_TIMEOUT);
  BOOST_CHECK_EQUAL(info.isTimeoutScheduled(), false);
}

BOOST_AUTO_TEST_SUITE_END() // TestFaceInfo

BOOST_AUTO_TEST_SUITE_END() // TestAsfStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace asf
} // namespace fw
} // namespace nfd
