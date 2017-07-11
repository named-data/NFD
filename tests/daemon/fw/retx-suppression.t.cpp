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

#include "fw/strategy.hpp"
#include "fw/retx-suppression-fixed.hpp"
#include "fw/retx-suppression-exponential.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestRetxSuppression, UnitTestTimeFixture)

BOOST_AUTO_TEST_CASE(Fixed)
{
  Forwarder forwarder;
  Pit& pit = forwarder.getPit();
  static const time::milliseconds MIN_RETX_INTERVAL(200);
  RetxSuppressionFixed rs(MIN_RETX_INTERVAL);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  shared_ptr<Interest> interest = makeInterest("ndn:/0JiimvmxK8");
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;

  const time::nanoseconds RETRANSMISSION_10P =
      time::duration_cast<time::nanoseconds>(MIN_RETX_INTERVAL * 0.1);

  // @ time 0
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::NEW);
  pitEntry->insertOrUpdateOutRecord(*face3, *interest);

  this->advanceClocks(RETRANSMISSION_10P, 5); // @ time 0.5 interval
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::SUPPRESS);
  pitEntry->insertOrUpdateInRecord(*face2, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::SUPPRESS);

  this->advanceClocks(RETRANSMISSION_10P, 6); // @ time 1.1 interval
  pitEntry->insertOrUpdateInRecord(*face2, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::FORWARD);
  // but strategy decides not to forward

  this->advanceClocks(RETRANSMISSION_10P, 1); // @ time 1.2 interval
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::FORWARD);
  // retransmission suppress shall still give clearance for forwarding
  pitEntry->insertOrUpdateOutRecord(*face3, *interest); // and strategy forwards

  this->advanceClocks(RETRANSMISSION_10P, 2); // @ time 1.4 interval
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::SUPPRESS);
  pitEntry->insertOrUpdateInRecord(*face2, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::SUPPRESS);
}

BOOST_AUTO_TEST_CASE(Exponential)
{
  Forwarder forwarder;
  Pit& pit = forwarder.getPit();
  RetxSuppressionExponential rs(time::milliseconds(10), 3.0, time::milliseconds(100));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  shared_ptr<Interest> interest = makeInterest("ndn:/smuVeQSW6q");
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;

  // @ 0ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::NEW);
  pitEntry->insertOrUpdateOutRecord(*face2, *interest);
  // suppression interval is 10ms, until 10ms

  this->advanceClocks(time::milliseconds(5)); // @ 5ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::SUPPRESS);
  // suppression interval is 10ms, until 10ms

  this->advanceClocks(time::milliseconds(6)); // @ 11ms
  // note: what happens at *exactly* 10ms does not matter so it's untested,
  // because in reality network timing won't be exact:
  // incoming Interest is processed either before or after 10ms point
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::FORWARD);
  pitEntry->insertOrUpdateOutRecord(*face2, *interest);
  // suppression interval is 30ms, until 41ms

  this->advanceClocks(time::milliseconds(25)); // @ 36ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::SUPPRESS);
  // suppression interval is 30ms, until 41ms

  this->advanceClocks(time::milliseconds(6)); // @ 42ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::FORWARD);
  // strategy decides not to forward, but suppression interval is increased nevertheless
  // suppression interval is 90ms, until 101ms

  this->advanceClocks(time::milliseconds(58)); // @ 100ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::SUPPRESS);
  // suppression interval is 90ms, until 101ms

  this->advanceClocks(time::milliseconds(3)); // @ 103ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::FORWARD);
  pitEntry->insertOrUpdateOutRecord(*face2, *interest);
  // suppression interval is 100ms, until 203ms

  this->advanceClocks(time::milliseconds(99)); // @ 202ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::SUPPRESS);
  // suppression interval is 100ms, until 203ms

  this->advanceClocks(time::milliseconds(2)); // @ 204ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerPitEntry(*pitEntry) == RetxSuppressionResult::FORWARD);
  pitEntry->insertOrUpdateOutRecord(*face2, *interest);
  // suppression interval is 100ms, until 304ms
}

BOOST_AUTO_TEST_CASE(ExponentialPerUpstream)
{
  Forwarder forwarder;
  Pit& pit = forwarder.getPit();
  RetxSuppressionExponential rs(time::milliseconds(10), 3.0, time::milliseconds(100));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  shared_ptr<Interest> interest = makeInterest("ndn:/covfefeW6q");
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;

  // @ 0ms
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerUpstream(*pitEntry, *face2) == RetxSuppressionResult::NEW);

  // Simluate forwarding an interest to face2
  pitEntry->insertOrUpdateOutRecord(*face2, *interest);
  this->advanceClocks(time::milliseconds(5)); // @ 5ms

  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerUpstream(*pitEntry, *face2) == RetxSuppressionResult::SUPPRESS);

  // Return NEW since no outRecord for face1
  pitEntry->insertOrUpdateInRecord(*face2, *interest);
  BOOST_CHECK(rs.decidePerUpstream(*pitEntry, *face1) == RetxSuppressionResult::NEW);

  this->advanceClocks(time::milliseconds(6)); // @ 11ms

  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK(rs.decidePerUpstream(*pitEntry, *face2) == RetxSuppressionResult::FORWARD);
  // Assume interest is sent and increment interval
  rs.incrementIntervalForOutRecord(*pitEntry->getOutRecord(*face2));

  pitEntry->insertOrUpdateInRecord(*face2, *interest);
  BOOST_CHECK(rs.decidePerUpstream(*pitEntry, *face2) == RetxSuppressionResult::SUPPRESS);
}

BOOST_AUTO_TEST_SUITE_END() // TestRetxSuppression
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
