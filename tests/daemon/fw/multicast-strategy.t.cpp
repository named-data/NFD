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

#include "fw/multicast-strategy.hpp"
#include "common/global.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "strategy-tester.hpp"

namespace nfd {
namespace fw {
namespace tests {

using MulticastStrategyTester = StrategyTester<MulticastStrategy>;
NFD_REGISTER_STRATEGY(MulticastStrategyTester);

class MulticastStrategyFixture : public GlobalIoTimeFixture
{
protected:
  MulticastStrategyFixture()
    : face1(make_shared<DummyFace>())
    , face2(make_shared<DummyFace>())
    , face3(make_shared<DummyFace>())
  {
    faceTable.add(face1);
    faceTable.add(face2);
    faceTable.add(face3);
  }

protected:
  FaceTable faceTable;
  Forwarder forwarder{faceTable};
  MulticastStrategyTester strategy{forwarder};
  Fib& fib{forwarder.getFib()};
  Pit& pit{forwarder.getPit()};

  shared_ptr<DummyFace> face1;
  shared_ptr<DummyFace> face2;
  shared_ptr<DummyFace> face3;
};

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestMulticastStrategy, MulticastStrategyFixture)

BOOST_AUTO_TEST_CASE(Forward2)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face1, 0);
  fib.addOrUpdateNextHop(fibEntry, *face2, 0);
  fib.addOrUpdateNextHop(fibEntry, *face3, 0);

  shared_ptr<Interest> interest = makeInterest("ndn:/H0D6i5fc");
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face3, *interest);

  strategy.afterReceiveInterest(FaceEndpoint(*face3, 0), *interest, pitEntry);
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.size(), 2);
  std::set<FaceId> sentInterestFaceIds;
  std::transform(strategy.sendInterestHistory.begin(), strategy.sendInterestHistory.end(),
                 std::inserter(sentInterestFaceIds, sentInterestFaceIds.end()),
                 [] (const MulticastStrategyTester::SendInterestArgs& args) {
                   return args.outFaceId;
                 });
  std::set<FaceId> expectedInterestFaceIds{face1->getId(), face2->getId()};
  BOOST_CHECK_EQUAL_COLLECTIONS(sentInterestFaceIds.begin(), sentInterestFaceIds.end(),
                                expectedInterestFaceIds.begin(), expectedInterestFaceIds.end());

  const time::nanoseconds TICK = time::duration_cast<time::nanoseconds>(
                                 MulticastStrategy::RETX_SUPPRESSION_INITIAL * 0.1);

  // downstream retransmits frequently, but the strategy should not send Interests
  // more often than DEFAULT_MIN_RETX_INTERVAL
  scheduler::EventId retxFrom4Evt;
  size_t nSentLast = strategy.sendInterestHistory.size();
  time::steady_clock::TimePoint timeSentLast = time::steady_clock::now();
  std::function<void()> periodicalRetxFrom4; // let periodicalRetxFrom4 lambda capture itself
  periodicalRetxFrom4 = [&] {
    pitEntry->insertOrUpdateInRecord(*face3, *interest);
    strategy.afterReceiveInterest(FaceEndpoint(*face3, 0), *interest, pitEntry);

    size_t nSent = strategy.sendInterestHistory.size();
    if (nSent > nSentLast) {
      // Multicast strategy should multicast the interest to other two faces
      BOOST_CHECK_EQUAL(nSent - nSentLast, 2);
      auto timeSent = time::steady_clock::now();
      BOOST_CHECK_GE(timeSent - timeSentLast, TICK * 8);
      nSentLast = nSent;
      timeSentLast = timeSent;
    }

    retxFrom4Evt = getScheduler().schedule(TICK * 5, periodicalRetxFrom4);
  };
  periodicalRetxFrom4();
  this->advanceClocks(TICK, MulticastStrategy::RETX_SUPPRESSION_MAX * 16);
  retxFrom4Evt.cancel();
}

BOOST_AUTO_TEST_CASE(RejectLoopback)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face1, 0);

  shared_ptr<Interest> interest = makeInterest("ndn:/H0D6i5fc");
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest);

  strategy.afterReceiveInterest(FaceEndpoint(*face1, 0), *interest, pitEntry);
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestMulticastStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
