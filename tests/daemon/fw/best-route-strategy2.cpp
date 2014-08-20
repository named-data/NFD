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

#include "fw/best-route-strategy2.hpp"
#include "strategy-tester.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwBestRouteStrategy2, BaseFixture)

BOOST_AUTO_TEST_CASE(Forward)
{
  LimitedIo limitedIo;
  Forwarder forwarder;
  typedef StrategyTester<fw::BestRouteStrategy2> BestRouteStrategy2Tester;
  BestRouteStrategy2Tester strategy(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face4 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face5 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  forwarder.addFace(face4);
  forwarder.addFace(face5);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face1, 10);
  fibEntry->addNextHop(face2, 20);
  fibEntry->addNextHop(face3, 30);

  shared_ptr<Interest> interest = makeInterest("ndn:/BzgFBchqA");
  Pit& pit = forwarder.getPit();
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;

  const time::nanoseconds RETRANSMISSION60  = time::duration_cast<time::nanoseconds>(
    fw::BestRouteStrategy2::MIN_RETRANSMISSION_INTERVAL * 0.6); // 60%
  const time::nanoseconds RETRANSMISSION120 = time::duration_cast<time::nanoseconds>(
    fw::BestRouteStrategy2::MIN_RETRANSMISSION_INTERVAL * 1.2); // 120%

  pitEntry->insertOrUpdateInRecord(face1, *interest);
  strategy.afterReceiveInterest(*face1, *interest, fibEntry, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.back().get<1>(), face2);
  // face1 is downstream so it cannot be used at this time

  limitedIo.run(LimitedIo::UNLIMITED_OPS, RETRANSMISSION60);
  pitEntry->insertOrUpdateInRecord(face4, *interest);
  strategy.afterReceiveInterest(*face4, *interest, fibEntry, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 1);
  // ignored similar Interest

  limitedIo.run(LimitedIo::UNLIMITED_OPS, RETRANSMISSION60);
  pitEntry->insertOrUpdateInRecord(face4, *interest);
  strategy.afterReceiveInterest(*face4, *interest, fibEntry, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.back().get<1>(), face1);
  // accepted retransmission, forward to an unused upstream

  limitedIo.run(LimitedIo::UNLIMITED_OPS, RETRANSMISSION120);
  pitEntry->insertOrUpdateInRecord(face5, *interest);
  strategy.afterReceiveInterest(*face5, *interest, fibEntry, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 3);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.back().get<1>(), face3);
  // accepted similar Interest from new downstream, forward to an unused upstream

  limitedIo.run(LimitedIo::UNLIMITED_OPS, RETRANSMISSION120);
  pitEntry->insertOrUpdateInRecord(face4, *interest);
  strategy.afterReceiveInterest(*face4, *interest, fibEntry, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 4);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.back().get<1>(), face2);
  // accepted retransmission, forward to an eligible upstream with earliest OutRecord

  limitedIo.run(LimitedIo::UNLIMITED_OPS, RETRANSMISSION60);
  pitEntry->insertOrUpdateInRecord(face5, *interest);
  strategy.afterReceiveInterest(*face5, *interest, fibEntry, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 4);
  // ignored retransmission

  fibEntry->removeNextHop(face1);

  limitedIo.run(LimitedIo::UNLIMITED_OPS, RETRANSMISSION60);
  pitEntry->insertOrUpdateInRecord(face5, *interest);
  strategy.afterReceiveInterest(*face5, *interest, fibEntry, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 5);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.back().get<1>(), face3);
  // accepted retransmission, forward to an eligible upstream with earliest OutRecord
  // face1 cannot be used because it's gone from FIB entry
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
