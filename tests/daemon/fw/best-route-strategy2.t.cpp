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

#include "fw/best-route-strategy2.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "strategy-tester.hpp"
#include "topology-tester.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)

class BestRouteStrategy2Fixture : public UnitTestTimeFixture
{
protected:
  BestRouteStrategy2Fixture()
    : strategy(forwarder)
    , fib(forwarder.getFib())
    , pit(forwarder.getPit())
    , face1(make_shared<DummyFace>())
    , face2(make_shared<DummyFace>())
    , face3(make_shared<DummyFace>())
    , face4(make_shared<DummyFace>())
    , face5(make_shared<DummyFace>())
  {
    forwarder.addFace(face1);
    forwarder.addFace(face2);
    forwarder.addFace(face3);
    forwarder.addFace(face4);
    forwarder.addFace(face5);
  }

public:
  Forwarder forwarder;
  StrategyTester<fw::BestRouteStrategy2> strategy;
  Fib& fib;
  Pit& pit;

  shared_ptr<DummyFace> face1;
  shared_ptr<DummyFace> face2;
  shared_ptr<DummyFace> face3;
  shared_ptr<DummyFace> face4;
  shared_ptr<DummyFace> face5;
};

BOOST_FIXTURE_TEST_SUITE(TestBestRouteStrategy2, BestRouteStrategy2Fixture)

BOOST_AUTO_TEST_CASE(Forward)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face1, 10);
  fibEntry.addNextHop(*face2, 20);
  fibEntry.addNextHop(*face3, 30);

  shared_ptr<Interest> interest = makeInterest("ndn:/BzgFBchqA");
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;

  const time::nanoseconds TICK = time::duration_cast<time::nanoseconds>(
    BestRouteStrategy2::RETX_SUPPRESSION_INITIAL * 0.1);

  // first Interest goes to nexthop with lowest FIB cost,
  // however face1 is downstream so it cannot be used
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  strategy.afterReceiveInterest(*face1, *interest, pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.back().outFaceId, face2->getId());

  // downstream retransmits frequently, but the strategy should not send Interests
  // more often than DEFAULT_MIN_RETX_INTERVAL
  scheduler::EventId retxFrom4Evt;
  size_t nSentLast = strategy.sendInterestHistory.size();
  time::steady_clock::TimePoint timeSentLast = time::steady_clock::now();
  function<void()> periodicalRetxFrom4; // let periodicalRetxFrom4 lambda capture itself
  periodicalRetxFrom4 = [&] {
    pitEntry->insertOrUpdateInRecord(*face4, *interest);
    strategy.afterReceiveInterest(*face4, *interest, pitEntry);

    size_t nSent = strategy.sendInterestHistory.size();
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
  this->advanceClocks(TICK, BestRouteStrategy2::RETX_SUPPRESSION_MAX * 16);
  scheduler::cancel(retxFrom4Evt);

  // nexthops for accepted retransmissions: follow FIB cost,
  // later forward to an eligible upstream with earliest out-record
  BOOST_REQUIRE_GE(strategy.sendInterestHistory.size(), 6);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[1].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[2].outFaceId, face3->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[3].outFaceId, face2->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[4].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[5].outFaceId, face3->getId());

  fibEntry.removeNextHop(*face1);

  strategy.sendInterestHistory.clear();
  for (int i = 0; i < 3; ++i) {
    this->advanceClocks(TICK, BestRouteStrategy2::RETX_SUPPRESSION_MAX * 2);
    pitEntry->insertOrUpdateInRecord(*face5, *interest);
    strategy.afterReceiveInterest(*face5, *interest, pitEntry);
  }
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 3);
  BOOST_CHECK_NE(strategy.sendInterestHistory[0].outFaceId, face1->getId());
  BOOST_CHECK_NE(strategy.sendInterestHistory[1].outFaceId, face1->getId());
  BOOST_CHECK_NE(strategy.sendInterestHistory[2].outFaceId, face1->getId());
  // face1 cannot be used because it's gone from FIB entry
}

BOOST_AUTO_TEST_SUITE(NoRouteNack) // send Nack-NoRoute if there's no usable FIB nexthop

class EmptyNextHopList
{
public:
  Name
  getInterestName()
  {
    return "/P";
  }

  void
  insertFibEntry(BestRouteStrategy2Fixture* fixture)
  {
    fixture->fib.insert(Name());
  }
};

class NextHopIsDownstream
{
public:
  Name
  getInterestName()
  {
    return "/P";
  }

  void
  insertFibEntry(BestRouteStrategy2Fixture* fixture)
  {
    fixture->fib.insert(Name()).first->addNextHop(*fixture->face1, 10);
  }
};

class NextHopViolatesScope
{
public:
  Name
  getInterestName()
  {
    return "/localhop/P";
  }

  void
  insertFibEntry(BestRouteStrategy2Fixture* fixture)
  {
    fixture->fib.insert("/localhop").first->addNextHop(*fixture->face2, 10);
    // face1 and face2 are both non-local; Interest from face1 cannot be forwarded to face2
  }
};

typedef boost::mpl::vector<EmptyNextHopList, NextHopIsDownstream, NextHopViolatesScope> NoRouteScenarios;

BOOST_AUTO_TEST_CASE_TEMPLATE(IncomingInterest, Scenario, NoRouteScenarios)
{
  Scenario scenario;
  scenario.insertFibEntry(this);

  shared_ptr<Interest> interest = makeInterest(scenario.getInterestName());
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest);

  strategy.afterReceiveInterest(*face1, *interest, pitEntry);

  BOOST_REQUIRE_EQUAL(strategy.rejectPendingInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory[0].pitInterest, pitEntry->getInterest());

  BOOST_REQUIRE_EQUAL(strategy.sendNackHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].header.getReason(), lp::NackReason::NO_ROUTE);
}

BOOST_AUTO_TEST_SUITE_END() // NoRouteNack

BOOST_AUTO_TEST_SUITE(IncomingNack)

BOOST_AUTO_TEST_CASE(OneUpstream) // one upstream, send Nack when Nack arrives
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face3, 10);
  fibEntry.addNextHop(*face4, 20);
  fibEntry.addNextHop(*face5, 30);

  shared_ptr<Interest> interest1 = makeInterest("/McQYjMbm", 992);
  shared_ptr<Interest> interest2 = makeInterest("/McQYjMbm", 114);
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest1).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest1);
  pitEntry->insertOrUpdateInRecord(*face2, *interest2);
  pitEntry->insertOrUpdateOutRecord(*face3, *interest1);

  lp::Nack nack3 = makeNack("/McQYjMbm", 992, lp::NackReason::CONGESTION);
  pitEntry->getOutRecord(*face3)->setIncomingNack(nack3);
  strategy.afterReceiveNack(*face3, nack3, pitEntry);

  BOOST_REQUIRE_EQUAL(strategy.sendNackHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].header.getReason(), lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[1].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[1].header.getReason(), lp::NackReason::CONGESTION);
  std::unordered_set<FaceId> nackFaceIds{strategy.sendNackHistory[0].outFaceId,
                                         strategy.sendNackHistory[1].outFaceId};
  std::unordered_set<FaceId> expectedNackFaceIds{face1->getId(), face2->getId()};
  BOOST_CHECK_EQUAL_COLLECTIONS(nackFaceIds.begin(), nackFaceIds.end(),
                                expectedNackFaceIds.begin(), expectedNackFaceIds.end());
}

BOOST_AUTO_TEST_CASE(TwoUpstreams) // two upstreams, send Nack when both Nacks arrive
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face3, 10);
  fibEntry.addNextHop(*face4, 20);
  fibEntry.addNextHop(*face5, 30);

  shared_ptr<Interest> interest1 = makeInterest("/aS9FAyUV19", 286);
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest1).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest1);
  pitEntry->insertOrUpdateOutRecord(*face3, *interest1);
  pitEntry->insertOrUpdateOutRecord(*face4, *interest1);

  lp::Nack nack3 = makeNack("/aS9FAyUV19", 286, lp::NackReason::CONGESTION);
  pitEntry->getOutRecord(*face3)->setIncomingNack(nack3);
  strategy.afterReceiveNack(*face3, nack3, pitEntry);

  BOOST_CHECK_EQUAL(strategy.sendNackHistory.size(), 0); // don't send Nack until all upstreams have Nacked

  lp::Nack nack4 = makeNack("/aS9FAyUV19", 286, lp::NackReason::CONGESTION);
  pitEntry->getOutRecord(*face4)->setIncomingNack(nack4);
  strategy.afterReceiveNack(*face4, nack4, pitEntry);

  BOOST_REQUIRE_EQUAL(strategy.sendNackHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].header.getReason(), lp::NackReason::CONGESTION);
}

BOOST_AUTO_TEST_CASE(Timeout) // two upstreams, one times out, don't send Nack
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face3, 10);
  fibEntry.addNextHop(*face4, 20);
  fibEntry.addNextHop(*face5, 30);

  shared_ptr<Interest> interest1 = makeInterest("/sIYw0TXWDj", 115);
  interest1->setInterestLifetime(time::milliseconds(400));
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest1).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest1);
  pitEntry->insertOrUpdateOutRecord(*face3, *interest1);

  this->advanceClocks(time::milliseconds(300));
  shared_ptr<Interest> interest2 = makeInterest("/sIYw0TXWDj", 223);
  pitEntry->insertOrUpdateInRecord(*face1, *interest2);
  pitEntry->insertOrUpdateOutRecord(*face4, *interest2);

  this->advanceClocks(time::milliseconds(200)); // face3 has timed out

  lp::Nack nack4 = makeNack("/sIYw0TXWDj", 223, lp::NackReason::CONGESTION);
  pitEntry->getOutRecord(*face4)->setIncomingNack(nack4);
  strategy.afterReceiveNack(*face4, nack4, pitEntry);

  BOOST_CHECK_EQUAL(strategy.sendNackHistory.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(LiveDeadlock, UnitTestTimeFixture) // #3033 note-7
{
  /*
   *           /----------\
   *           | producer |
   *           \----------/
   *                |
   *              +---+
   *              | P |
   *              +---+
   *                |
   *           failed link
   *                |
   *              +---+
   *              | R |
   *              +---+
   *             ^     ^
   *            /       \
   *           /         \
   *        +---+       +---+
   *        | B | <---> | C |
   *        +---+       +---+
   *          ^           ^
   *          |           |
   *          |           |
   *        +---+       +---+
   *        | A |       | D |
   *        +---+       +---+
   *          ^           ^
   *          |           |
   *  /----------\     /----------\
   *  | consumer |     | consumer |
   *  \----------/     \----------/
   */

  TopologyTester topo;
  TopologyNode nodeP = topo.addForwarder("P"),
               nodeR = topo.addForwarder("R"),
               nodeA = topo.addForwarder("A"),
               nodeB = topo.addForwarder("B"),
               nodeC = topo.addForwarder("C"),
               nodeD = topo.addForwarder("D");

  for (TopologyNode node : {nodeP, nodeR, nodeA, nodeB, nodeC, nodeD}) {
    topo.setStrategy<BestRouteStrategy2>(node);
  }

  const time::milliseconds LINK_DELAY(10);
  shared_ptr<TopologyLink> linkPR = topo.addLink("PR", LINK_DELAY, {nodeP, nodeR}),
                           linkRB = topo.addLink("RB", LINK_DELAY, {nodeR, nodeB}),
                           linkRC = topo.addLink("RC", LINK_DELAY, {nodeR, nodeC}),
                           linkBC = topo.addLink("BC", LINK_DELAY, {nodeB, nodeC}),
                           linkBA = topo.addLink("BA", LINK_DELAY, {nodeB, nodeA}),
                           linkCD = topo.addLink("CD", LINK_DELAY, {nodeC, nodeD});

  // TODO register the prefix on R->P but then set the face DOWN
  // topo.registerPrefix(nodeR, linkPR->getFace(nodeR), "ndn:/P", 10);
  topo.registerPrefix(nodeB, linkRB->getFace(nodeB), "ndn:/P", 20);
  topo.registerPrefix(nodeB, linkBC->getFace(nodeB), "ndn:/P", 30);
  topo.registerPrefix(nodeC, linkRC->getFace(nodeC), "ndn:/P", 20);
  topo.registerPrefix(nodeC, linkBC->getFace(nodeC), "ndn:/P", 30);
  topo.registerPrefix(nodeA, linkBA->getFace(nodeA), "ndn:/P", 30);
  topo.registerPrefix(nodeD, linkCD->getFace(nodeD), "ndn:/P", 30);

  ndn::Face& appA = topo.addAppFace("A", nodeA)->getClientFace();
  ndn::Face& appD = topo.addAppFace("D", nodeD)->getClientFace();

  int nNacksA = 0, nNacksD = 0;
  appA.expressInterest(Interest("/P/1"), bind([]{}), bind([&nNacksA]{ ++nNacksA; }), bind([]{}));
  appD.expressInterest(Interest("/P/1"), bind([]{}), bind([&nNacksD]{ ++nNacksD; }), bind([]{}));
  this->advanceClocks(time::milliseconds(1), time::milliseconds(5));
  appA.expressInterest(Interest("/P/1"), bind([]{}), bind([&nNacksA]{ ++nNacksA; }), bind([]{}));
  appD.expressInterest(Interest("/P/1"), bind([]{}), bind([&nNacksD]{ ++nNacksD; }), bind([]{}));
  this->advanceClocks(time::milliseconds(1), time::milliseconds(100));

  // As long as at least one Nack arrives at each client, strategy behavior is correct.
  // Whether both Interests are Nacked is a client face behavior, not strategy behavior.
  BOOST_CHECK_GT(nNacksA, 0);
  BOOST_CHECK_GT(nNacksD, 0);
}

template<lp::NackReason X, lp::NackReason Y, lp::NackReason R>
struct NackReasonCombination
{
  lp::NackReason
  getX() const
  {
    return X;
  }

  lp::NackReason
  getY() const
  {
    return Y;
  }

  lp::NackReason
  getExpectedResult() const
  {
    return R;
  }
};

typedef boost::mpl::vector<
    NackReasonCombination<lp::NackReason::CONGESTION, lp::NackReason::CONGESTION, lp::NackReason::CONGESTION>,
    NackReasonCombination<lp::NackReason::CONGESTION, lp::NackReason::DUPLICATE, lp::NackReason::CONGESTION>,
    NackReasonCombination<lp::NackReason::CONGESTION, lp::NackReason::NO_ROUTE, lp::NackReason::CONGESTION>,
    NackReasonCombination<lp::NackReason::DUPLICATE, lp::NackReason::CONGESTION, lp::NackReason::CONGESTION>,
    NackReasonCombination<lp::NackReason::DUPLICATE, lp::NackReason::DUPLICATE, lp::NackReason::DUPLICATE>,
    NackReasonCombination<lp::NackReason::DUPLICATE, lp::NackReason::NO_ROUTE, lp::NackReason::DUPLICATE>,
    NackReasonCombination<lp::NackReason::NO_ROUTE, lp::NackReason::CONGESTION, lp::NackReason::CONGESTION>,
    NackReasonCombination<lp::NackReason::NO_ROUTE, lp::NackReason::DUPLICATE, lp::NackReason::DUPLICATE>,
    NackReasonCombination<lp::NackReason::NO_ROUTE, lp::NackReason::NO_ROUTE, lp::NackReason::NO_ROUTE>
  > NackReasonCombinations;

BOOST_AUTO_TEST_CASE_TEMPLATE(CombineReasons, Combination, NackReasonCombinations)
{
  Combination combination;

  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face3, 10);
  fibEntry.addNextHop(*face4, 20);
  fibEntry.addNextHop(*face5, 30);

  shared_ptr<Interest> interest1 = makeInterest("/F6sEwB24I", 282);
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest1).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest1);
  pitEntry->insertOrUpdateOutRecord(*face3, *interest1);
  pitEntry->insertOrUpdateOutRecord(*face4, *interest1);

  lp::Nack nack3 = makeNack("/F6sEwB24I", 282, combination.getX());
  pitEntry->getOutRecord(*face3)->setIncomingNack(nack3);
  strategy.afterReceiveNack(*face3, nack3, pitEntry);

  BOOST_CHECK_EQUAL(strategy.sendNackHistory.size(), 0);

  lp::Nack nack4 = makeNack("/F6sEwB24I", 282, combination.getY());
  pitEntry->getOutRecord(*face4)->setIncomingNack(nack4);
  strategy.afterReceiveNack(*face4, nack4, pitEntry);

  BOOST_REQUIRE_EQUAL(strategy.sendNackHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].header.getReason(), combination.getExpectedResult());
}

BOOST_AUTO_TEST_SUITE_END() // IncomingNack

BOOST_AUTO_TEST_SUITE_END() // TestBestRouteStrategy2
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
