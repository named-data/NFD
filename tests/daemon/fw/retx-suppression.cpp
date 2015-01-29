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

#include "fw/retx-suppression.hpp"
#include "fw/retx-suppression-fixed.hpp"
#include "strategy-tester.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace tests {

using fw::RetxSuppression;
using fw::RetxSuppressionFixed;

BOOST_FIXTURE_TEST_SUITE(FwRetxSuppression, UnitTestTimeFixture)

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
  pitEntry->insertOrUpdateInRecord(face1, *interest);
  BOOST_CHECK_EQUAL(rs.decide(*face1, *interest, *pitEntry), RetxSuppression::NEW);
  pitEntry->insertOrUpdateOutRecord(face3, *interest);

  this->advanceClocks(RETRANSMISSION_10P, 5); // @ time 0.5 interval
  pitEntry->insertOrUpdateInRecord(face1, *interest);
  BOOST_CHECK_EQUAL(rs.decide(*face1, *interest, *pitEntry), RetxSuppression::SUPPRESS);
  pitEntry->insertOrUpdateInRecord(face2, *interest);
  BOOST_CHECK_EQUAL(rs.decide(*face2, *interest, *pitEntry), RetxSuppression::SUPPRESS);

  this->advanceClocks(RETRANSMISSION_10P, 6); // @ time 1.1 interval
  pitEntry->insertOrUpdateInRecord(face2, *interest);
  BOOST_CHECK_EQUAL(rs.decide(*face2, *interest, *pitEntry), RetxSuppression::FORWARD);
  // but strategy decides not to forward

  this->advanceClocks(RETRANSMISSION_10P, 1); // @ time 1.2 interval
  pitEntry->insertOrUpdateInRecord(face1, *interest);
  BOOST_CHECK_EQUAL(rs.decide(*face1, *interest, *pitEntry), RetxSuppression::FORWARD);
  // retransmission suppress shall still give clearance for forwarding
  pitEntry->insertOrUpdateOutRecord(face3, *interest); // and strategy forwards

  this->advanceClocks(RETRANSMISSION_10P, 2); // @ time 1.4 interval
  pitEntry->insertOrUpdateInRecord(face1, *interest);
  BOOST_CHECK_EQUAL(rs.decide(*face1, *interest, *pitEntry), RetxSuppression::SUPPRESS);
  pitEntry->insertOrUpdateInRecord(face2, *interest);
  BOOST_CHECK_EQUAL(rs.decide(*face2, *interest, *pitEntry), RetxSuppression::SUPPRESS);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
