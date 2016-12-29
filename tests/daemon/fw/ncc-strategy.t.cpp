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

#include "fw/ncc-strategy.hpp"
#include "strategy-tester.hpp"
#include "topology-tester.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "tests/limited-io.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

typedef StrategyTester<NccStrategy> NccStrategyTester;
NFD_REGISTER_STRATEGY(NccStrategyTester);

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestNccStrategy, UnitTestTimeFixture)

BOOST_AUTO_TEST_CASE(FavorRespondingUpstream)
{
  // NccStrategy is fairly complex.
  // The most important property is: it remembers which upstream is the fastest to return Data,
  // and favors this upstream in subsequent Interests.

  LimitedIo limitedIo(this);
  Forwarder forwarder;
  NccStrategyTester& strategy = choose<NccStrategyTester>(forwarder);
  strategy.afterAction.connect(bind(&LimitedIo::afterOp, &limitedIo));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face1, 10);
  fibEntry.addNextHop(*face2, 20);

  Pit& pit = forwarder.getPit();

  // first Interest: strategy knows nothing and follows routing
  shared_ptr<Interest> interest1p = makeInterest("ndn:/0Jm1ajrW/%00");
  Interest& interest1 = *interest1p;
  interest1.setInterestLifetime(time::milliseconds(8000));
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(interest1).first;

  pitEntry1->insertOrUpdateInRecord(*face3, interest1);
  strategy.afterReceiveInterest(*face3, interest1, pitEntry1);

  // forwards to face1 because routing says it's best
  // (no io run here: afterReceiveInterest has already sent the Interest)
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[0].outFaceId, face1->getId());

  // forwards to face2 because face1 doesn't respond
  limitedIo.run(1, time::milliseconds(500), time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[1].outFaceId, face2->getId());

  // face2 responds
  shared_ptr<Data> data1p = makeData("ndn:/0Jm1ajrW/%00");
  Data& data1 = *data1p;
  strategy.beforeSatisfyInterest(pitEntry1, *face2, data1);
  this->advanceClocks(time::milliseconds(10), time::milliseconds(500));

  // second Interest: strategy knows face2 is best
  shared_ptr<Interest> interest2p = makeInterest("ndn:/0Jm1ajrW/%00%01");
  Interest& interest2 = *interest2p;
  interest2.setInterestLifetime(time::milliseconds(8000));
  shared_ptr<pit::Entry> pitEntry2 = pit.insert(interest2).first;

  pitEntry2->insertOrUpdateInRecord(*face3, interest2);
  strategy.afterReceiveInterest(*face3, interest2, pitEntry2);

  // forwards to face2 because it responds previously
  this->advanceClocks(time::milliseconds(1));
  BOOST_REQUIRE_GE(strategy.sendInterestHistory.size(), 3);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[2].outFaceId, face2->getId());
}

BOOST_AUTO_TEST_CASE(Bug1853)
{
  Forwarder forwarder;
  NccStrategyTester& strategy = choose<NccStrategyTester>(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face1, 10);

  Pit& pit = forwarder.getPit();

  // first Interest: strategy follows routing and forwards to face1
  shared_ptr<Interest> interest1 = makeInterest("ndn:/nztwIvHX/%00");
  interest1->setInterestLifetime(time::milliseconds(8000));
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;

  pitEntry1->insertOrUpdateInRecord(*face3, *interest1);
  strategy.afterReceiveInterest(*face3, *interest1, pitEntry1);

  this->advanceClocks(time::milliseconds(1));
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[0].outFaceId, face1->getId());

  // face1 responds
  shared_ptr<Data> data1 = makeData("ndn:/nztwIvHX/%00");
  strategy.beforeSatisfyInterest(pitEntry1, *face1, *data1);
  this->advanceClocks(time::milliseconds(10), time::milliseconds(500));

  // second Interest: bestFace is face1, nUpstreams becomes 0,
  // therefore pitEntryInfo->maxInterval cannot be calculated from deferRange and nUpstreams
  shared_ptr<Interest> interest2 = makeInterest("ndn:/nztwIvHX/%01");
  interest2->setInterestLifetime(time::milliseconds(8000));
  shared_ptr<pit::Entry> pitEntry2 = pit.insert(*interest2).first;

  pitEntry2->insertOrUpdateInRecord(*face3, *interest2);
  strategy.afterReceiveInterest(*face3, *interest2, pitEntry2);

  // FIB entry is changed before doPropagate executes
  fibEntry.addNextHop(*face2, 20);
  this->advanceClocks(time::milliseconds(10), time::milliseconds(1000));// should not crash
}

BOOST_AUTO_TEST_CASE(Bug1961)
{
  LimitedIo limitedIo(this);
  Forwarder forwarder;
  NccStrategyTester& strategy = choose<NccStrategyTester>(forwarder);
  strategy.afterAction.connect(bind(&LimitedIo::afterOp, &limitedIo));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face1, 10);
  fibEntry.addNextHop(*face2, 20);

  Pit& pit = forwarder.getPit();

  // first Interest: strategy forwards to face1 and face2
  shared_ptr<Interest> interest1 = makeInterest("ndn:/seRMz5a6/%00");
  interest1->setInterestLifetime(time::milliseconds(2000));
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;

  pitEntry1->insertOrUpdateInRecord(*face3, *interest1);
  strategy.afterReceiveInterest(*face3, *interest1, pitEntry1);
  limitedIo.run(2 - strategy.sendInterestHistory.size(),
                time::milliseconds(2000), time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[0].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[1].outFaceId, face2->getId());

  // face1 responds
  shared_ptr<Data> data1 = makeData("ndn:/seRMz5a6/%00");
  strategy.beforeSatisfyInterest(pitEntry1, *face1, *data1);
  pitEntry1->clearInRecords();
  this->advanceClocks(time::milliseconds(10));
  // face2 also responds
  strategy.beforeSatisfyInterest(pitEntry1, *face2, *data1);
  this->advanceClocks(time::milliseconds(10));

  // second Interest: bestFace should be face 1
  shared_ptr<Interest> interest2 = makeInterest("ndn:/seRMz5a6/%01");
  interest2->setInterestLifetime(time::milliseconds(2000));
  shared_ptr<pit::Entry> pitEntry2 = pit.insert(*interest2).first;

  pitEntry2->insertOrUpdateInRecord(*face3, *interest2);
  strategy.afterReceiveInterest(*face3, *interest2, pitEntry2);
  limitedIo.run(3 - strategy.sendInterestHistory.size(),
                time::milliseconds(2000), time::milliseconds(10));

  BOOST_REQUIRE_GE(strategy.sendInterestHistory.size(), 3);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[2].outFaceId, face1->getId());
}

BOOST_AUTO_TEST_CASE(Bug1971)
{
  LimitedIo limitedIo(this);
  Forwarder forwarder;
  NccStrategyTester& strategy = choose<NccStrategyTester>(forwarder);
  strategy.afterAction.connect(bind(&LimitedIo::afterOp, &limitedIo));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face2, 10);

  Pit& pit = forwarder.getPit();

  // first Interest: strategy forwards to face2
  shared_ptr<Interest> interest1 = makeInterest("ndn:/M4mBXCsd");
  interest1->setInterestLifetime(time::milliseconds(2000));
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;

  pitEntry1->insertOrUpdateInRecord(*face1, *interest1);
  strategy.afterReceiveInterest(*face1, *interest1, pitEntry1);
  limitedIo.run(1 - strategy.sendInterestHistory.size(),
                time::milliseconds(2000), time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[0].outFaceId, face2->getId());

  // face2 responds
  shared_ptr<Data> data1 = makeData("ndn:/M4mBXCsd");
  data1->setFreshnessPeriod(time::milliseconds(5));
  strategy.beforeSatisfyInterest(pitEntry1, *face2, *data1);
  pitEntry1->deleteOutRecord(*face2);
  pitEntry1->clearInRecords();
  this->advanceClocks(time::milliseconds(10));

  // similar Interest: strategy should still forward it
  pitEntry1->insertOrUpdateInRecord(*face1, *interest1);
  strategy.afterReceiveInterest(*face1, *interest1, pitEntry1);
  limitedIo.run(2 - strategy.sendInterestHistory.size(),
                time::milliseconds(2000), time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[1].outFaceId, face2->getId());
}

BOOST_AUTO_TEST_CASE(Bug1998)
{
  Forwarder forwarder;
  NccStrategyTester& strategy = choose<NccStrategyTester>(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face1, 10); // face1 is top-ranked nexthop
  fibEntry.addNextHop(*face2, 20);

  Pit& pit = forwarder.getPit();

  // Interest comes from face1, which is sole downstream
  shared_ptr<Interest> interest1 = makeInterest("ndn:/tFy5HzUzD4");
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;
  pitEntry1->insertOrUpdateInRecord(*face1, *interest1);

  strategy.afterReceiveInterest(*face1, *interest1, pitEntry1);

  // Interest shall go to face2, not loop back to face1
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[0].outFaceId, face2->getId());
}

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(PredictionAdjustment, 1)
BOOST_AUTO_TEST_CASE(PredictionAdjustment) // Bug 3411
{
  /*
   * /----------\
   * | consumer |
   * \----------/
   *     |
   *     |
   *   +---+  5ms  +---+
   *   | A |-------| B |
   *   +---+       +---+
   *     |           |
   *     | 5ms       | 15ms/5ms
   *     |           |
   *   +---+       +---+
   *   | C |-------| D |
   *   +---+ 15ms  +---+
   *         /5ms    |
   *                 |
   *         /----------\
   *         | producer |
   *         \----------/
   */

  TopologyTester topo;
  TopologyNode nodeA = topo.addForwarder("A"),
               nodeB = topo.addForwarder("B"),
               nodeC = topo.addForwarder("C"),
               nodeD = topo.addForwarder("D");

  for (TopologyNode node : {nodeA, nodeB, nodeC, nodeD}) {
    topo.setStrategy<NccStrategy>(node);
  }

  shared_ptr<TopologyLink> linkAB = topo.addLink("AB", time::milliseconds( 5), {nodeA, nodeB}),
                           linkAC = topo.addLink("AC", time::milliseconds( 5), {nodeA, nodeC}),
                           linkBD = topo.addLink("BD", time::milliseconds(15), {nodeB, nodeD}),
                           linkCD = topo.addLink("CD", time::milliseconds(15), {nodeC, nodeD});

  topo.registerPrefix(nodeA, linkAB->getFace(nodeA), "ndn:/P");
  topo.registerPrefix(nodeA, linkAC->getFace(nodeA), "ndn:/P");
  topo.registerPrefix(nodeB, linkBD->getFace(nodeB), "ndn:/P");
  topo.registerPrefix(nodeC, linkCD->getFace(nodeC), "ndn:/P");

  shared_ptr<TopologyAppLink> producer = topo.addAppFace("producer", nodeD, "ndn:/P");
  topo.addEchoProducer(producer->getClientFace());

  shared_ptr<TopologyAppLink> consumer = topo.addAppFace("consumer", nodeA);
  topo.addIntervalConsumer(consumer->getClientFace(), "ndn:/P",
                           time::milliseconds(100), 300);

  auto getMeInfo = [&] () -> NccStrategy::MeasurementsEntryInfo* {
    Measurements& measurements = topo.getForwarder(nodeA).getMeasurements();
    measurements::Entry* me = measurements.findExactMatch("ndn:/P");
    return me == nullptr ? nullptr : me->getStrategyInfo<NccStrategy::MeasurementsEntryInfo>();
  };

  // NccStrategy starts with an exploration period when the algorithm adjusts
  // its prediction to converge near the path RTT.
  bool isExplorationFinished = false;
  const time::milliseconds TRUE_RTT1(40);
  bool isLastPredictionUnder = true;
  int nPredictionCrossings = 0;
  for (int i = 0; i < 10000; ++i) {
    this->advanceClocks(time::milliseconds(5));
    auto meInfo = getMeInfo();
    if (meInfo == nullptr) {
      continue;
    }

    if ((isLastPredictionUnder && meInfo->prediction > TRUE_RTT1) ||
        (!isLastPredictionUnder && meInfo->prediction < TRUE_RTT1)) {
      isLastPredictionUnder = !isLastPredictionUnder;
      if (++nPredictionCrossings > 6) {
        isExplorationFinished = true;
        BOOST_TEST_MESSAGE("exploration finishes after " << (i * 5) << "ms");
        break;
      }
    }
  }
  BOOST_REQUIRE_MESSAGE(isExplorationFinished, "exploration does not finish in 50000ms");

  // NccStrategy has selected one path as the best.
  // When we reduce the RTT of the other path, ideally it should be selected as the best face.
  // However, this won't happen due to a weakness in NccStrategy.
  // See http://redmine.named-data.net/issues/3411#note-4
  shared_ptr<Face> bestFace1 = getMeInfo()->bestFace.lock();
  if (bestFace1.get() == &linkAB->getFace(nodeA)) {
    linkCD->setDelay(time::milliseconds(5));
  }
  else if (bestFace1.get() == &linkAC->getFace(nodeA)) {
    linkBD->setDelay(time::milliseconds(5));
  }
  else {
    BOOST_FAIL("unexpected best face");
  }

  bool isNewBestChosen = false;
  for (int i = 0; i < 10000; ++i) {
    this->advanceClocks(time::milliseconds(5));
    auto meInfo = getMeInfo();
    if (meInfo == nullptr) {
      continue;
    }

    if (meInfo->bestFace.lock() != bestFace1) {
      isNewBestChosen = true;
      BOOST_TEST_MESSAGE("new best face is found after " << (i * 5) << "ms");
      break;
    }
  }
  BOOST_CHECK_MESSAGE(isNewBestChosen, "new best face is not found in 50000ms"); // expected failure
}

BOOST_AUTO_TEST_SUITE_END() // TestNccStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
