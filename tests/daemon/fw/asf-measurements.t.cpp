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

#include "fw/asf-measurements.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace fw {
namespace asf {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_AUTO_TEST_SUITE(TestAsfMeasurements)

BOOST_FIXTURE_TEST_CASE(FaceInfo, GlobalIoTimeFixture)
{
  using asf::FaceInfo;
  FaceInfo info(nullptr);

  BOOST_CHECK_EQUAL(info.getLastRtt(), FaceInfo::RTT_NO_MEASUREMENT);
  BOOST_CHECK_EQUAL(info.getSrtt(), FaceInfo::RTT_NO_MEASUREMENT);

  info.recordRtt(100_ms);
  Name interestName("/ndn/interest");

  // Receive Interest and forward to next hop; should update RTO information
  BOOST_CHECK_EQUAL(info.isTimeoutScheduled(), false);
  auto rto = info.scheduleTimeout(interestName, []{});
  BOOST_CHECK_EQUAL(info.isTimeoutScheduled(), true);
  BOOST_CHECK_EQUAL(rto, 300_ms);

  // Receive Data
  time::nanoseconds rtt(5_ms);
  this->advanceClocks(5_ms);
  info.recordRtt(rtt);
  info.cancelTimeout(interestName);

  BOOST_CHECK_EQUAL(info.getLastRtt(), rtt);
  BOOST_CHECK_EQUAL(info.getSrtt(), 88125_us);

  // Send out another Interest which times out
  rto = info.scheduleTimeout(interestName, []{});
  BOOST_CHECK_EQUAL(rto, 333125_us);

  auto previousSrtt = info.getSrtt();
  info.recordTimeout(interestName);

  BOOST_CHECK_EQUAL(info.getLastRtt(), FaceInfo::RTT_TIMEOUT);
  BOOST_CHECK_EQUAL(info.getSrtt(), previousSrtt);
  BOOST_CHECK_EQUAL(info.isTimeoutScheduled(), false);
}

BOOST_FIXTURE_TEST_CASE(NamespaceInfo, GlobalIoTimeFixture)
{
  using asf::NamespaceInfo;
  NamespaceInfo info(nullptr);

  BOOST_CHECK(info.getFaceInfo(1234) == nullptr);

  auto& faceInfo = info.getOrCreateFaceInfo(1234);
  BOOST_CHECK(info.getFaceInfo(1234) == &faceInfo);

  this->advanceClocks(AsfMeasurements::MEASUREMENTS_LIFETIME + 1_s);
  BOOST_CHECK(info.getFaceInfo(1234) == nullptr); // expired
}

BOOST_AUTO_TEST_SUITE_END() // TestAsfStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace asf
} // namespace fw
} // namespace nfd
