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

#include "fw/best-route-strategy2.hpp"
#include "strategy-tester.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_FIXTURE_TEST_SUITE(FwBestRouteStrategy2, UnitTestTimeFixture)

BOOST_AUTO_TEST_CASE(Forward)
{
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

  const time::nanoseconds TICK = time::duration_cast<time::nanoseconds>(
    fw::RetxSuppressionExponential::DEFAULT_INITIAL_INTERVAL * 0.1);

  // first Interest goes to nexthop with lowest FIB cost,
  // however face1 is downstream so it cannot be used
  pitEntry->insertOrUpdateInRecord(face1, *interest);
  strategy.afterReceiveInterest(*face1, *interest, fibEntry, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.back().get<1>(), face2);

  // downstream retransmits frequently, but the strategy should not send Interests
  // more often than DEFAULT_MIN_RETX_INTERVAL
  scheduler::EventId retxFrom4Evt;
  size_t nSentLast = strategy.m_sendInterestHistory.size();
  time::steady_clock::TimePoint timeSentLast = time::steady_clock::now();
  function<void()> periodicalRetxFrom4; // let periodicalRetxFrom4 lambda capture itself
  periodicalRetxFrom4 = [&] {
    pitEntry->insertOrUpdateInRecord(face4, *interest);
    strategy.afterReceiveInterest(*face4, *interest, fibEntry, pitEntry);

    size_t nSent = strategy.m_sendInterestHistory.size();
    if (nSent > nSentLast) {
      BOOST_CHECK_EQUAL(nSent - nSentLast, 1);
      time::steady_clock::TimePoint timeSent = time::steady_clock::now();
      BOOST_CHECK_GE(timeSent - timeSentLast, TICK * 8);
      nSentLast = nSent;
      timeSentLast = timeSent;
    }

    retxFrom4Evt = scheduler::schedule(TICK * 5, periodicalRetxFrom4);
  };
  periodicalRetxFrom4();
  this->advanceClocks(TICK, fw::RetxSuppressionExponential::DEFAULT_MAX_INTERVAL * 16);
  scheduler::cancel(retxFrom4Evt);

  // nexthops for accepted retransmissions: follow FIB cost,
  // later forward to an eligible upstream with earliest OutRecord
  BOOST_REQUIRE_GE(strategy.m_sendInterestHistory.size(), 6);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory[1].get<1>(), face1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory[2].get<1>(), face3);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory[3].get<1>(), face2);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory[4].get<1>(), face1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory[5].get<1>(), face3);

  fibEntry->removeNextHop(face1);

  strategy.m_sendInterestHistory.clear();
  for (int i = 0; i < 3; ++i) {
    this->advanceClocks(TICK, fw::RetxSuppressionExponential::DEFAULT_MAX_INTERVAL * 2);
    pitEntry->insertOrUpdateInRecord(face5, *interest);
    strategy.afterReceiveInterest(*face5, *interest, fibEntry, pitEntry);
  }
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 3);
  BOOST_CHECK_NE(strategy.m_sendInterestHistory[0].get<1>(), face1);
  BOOST_CHECK_NE(strategy.m_sendInterestHistory[1].get<1>(), face1);
  BOOST_CHECK_NE(strategy.m_sendInterestHistory[2].get<1>(), face1);
  // face1 cannot be used because it's gone from FIB entry
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace fw
} // namespace nfd
