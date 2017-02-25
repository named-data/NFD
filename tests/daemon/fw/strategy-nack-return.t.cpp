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

/** \file
 *  This test suite tests incoming Nack processing in strategies.
 */

// Strategies implementing recommended Nack processing procedure, sorted alphabetically.
#include "fw/best-route-strategy2.hpp"
#include "fw/multicast-strategy.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"
#include "choose-strategy.hpp"
#include "strategy-tester.hpp"
#include "topology-tester.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include <boost/mpl/vector.hpp>

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)

template<typename S>
class StrategyNackReturnFixture : public UnitTestTimeFixture
{
public:
  StrategyNackReturnFixture()
    : limitedIo(this)
    , strategy(choose<StrategyTester<S>>(forwarder))
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
  LimitedIo limitedIo;

  Forwarder forwarder;
  StrategyTester<S>& strategy;
  Fib& fib;
  Pit& pit;

  shared_ptr<Face> face1;
  shared_ptr<Face> face2;
  shared_ptr<Face> face3;
  shared_ptr<Face> face4;
  shared_ptr<Face> face5;
};

BOOST_AUTO_TEST_SUITE(TestStrategyNackReturn)

using Strategies = boost::mpl::vector<
  BestRouteStrategy2,
  MulticastStrategy
>;

// one upstream, send Nack when Nack arrives
BOOST_FIXTURE_TEST_CASE_TEMPLATE(OneUpstream,
                                 S, Strategies, StrategyNackReturnFixture<S>)
{
  fib::Entry& fibEntry = *this->fib.insert(Name()).first;
  fibEntry.addNextHop(*this->face3, 10);
  fibEntry.addNextHop(*this->face4, 20);
  fibEntry.addNextHop(*this->face5, 30);

  shared_ptr<Interest> interest1 = makeInterest("/McQYjMbm", 992);
  shared_ptr<Interest> interest2 = makeInterest("/McQYjMbm", 114);
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest1).first;
  pitEntry->insertOrUpdateInRecord(*this->face1, *interest1);
  pitEntry->insertOrUpdateInRecord(*this->face2, *interest2);
  pitEntry->insertOrUpdateOutRecord(*this->face3, *interest1);

  lp::Nack nack3 = makeNack("/McQYjMbm", 992, lp::NackReason::CONGESTION);
  pitEntry->getOutRecord(*this->face3)->setIncomingNack(nack3);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveNack(*this->face3, nack3, pitEntry); },
    this->limitedIo, 2));

  BOOST_REQUIRE_EQUAL(this->strategy.sendNackHistory.size(), 2);
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[0].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[0].header.getReason(), lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[1].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[1].header.getReason(), lp::NackReason::CONGESTION);
  std::set<FaceId> nackFaceIds{this->strategy.sendNackHistory[0].outFaceId,
                                         this->strategy.sendNackHistory[1].outFaceId};
  std::set<FaceId> expectedNackFaceIds{this->face1->getId(), this->face2->getId()};
  BOOST_CHECK_EQUAL_COLLECTIONS(nackFaceIds.begin(), nackFaceIds.end(),
                                expectedNackFaceIds.begin(), expectedNackFaceIds.end());
}

// two upstreams, send Nack when both Nacks arrive
BOOST_FIXTURE_TEST_CASE_TEMPLATE(TwoUpstreams,
                                 S, Strategies, StrategyNackReturnFixture<S>)
{
  fib::Entry& fibEntry = *this->fib.insert(Name()).first;
  fibEntry.addNextHop(*this->face3, 10);
  fibEntry.addNextHop(*this->face4, 20);
  fibEntry.addNextHop(*this->face5, 30);

  shared_ptr<Interest> interest1 = makeInterest("/aS9FAyUV19", 286);
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest1).first;
  pitEntry->insertOrUpdateInRecord(*this->face1, *interest1);
  pitEntry->insertOrUpdateOutRecord(*this->face3, *interest1);
  pitEntry->insertOrUpdateOutRecord(*this->face4, *interest1);

  lp::Nack nack3 = makeNack("/aS9FAyUV19", 286, lp::NackReason::CONGESTION);
  pitEntry->getOutRecord(*this->face3)->setIncomingNack(nack3);
  this->strategy.afterReceiveNack(*this->face3, nack3, pitEntry);

  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.size(), 0); // don't send Nack until all upstreams have Nacked

  lp::Nack nack4 = makeNack("/aS9FAyUV19", 286, lp::NackReason::CONGESTION);
  pitEntry->getOutRecord(*this->face4)->setIncomingNack(nack4);
  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveNack(*this->face4, nack4, pitEntry); },
    this->limitedIo));

  BOOST_REQUIRE_EQUAL(this->strategy.sendNackHistory.size(), 1);
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[0].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[0].outFaceId, this->face1->getId());
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[0].header.getReason(), lp::NackReason::CONGESTION);
}

// two upstreams, one times out, don't send Nack
BOOST_FIXTURE_TEST_CASE_TEMPLATE(Timeout,
                                 S, Strategies, StrategyNackReturnFixture<S>)
{
  fib::Entry& fibEntry = *this->fib.insert(Name()).first;
  fibEntry.addNextHop(*this->face3, 10);
  fibEntry.addNextHop(*this->face4, 20);
  fibEntry.addNextHop(*this->face5, 30);

  shared_ptr<Interest> interest1 = makeInterest("/sIYw0TXWDj", 115);
  interest1->setInterestLifetime(time::milliseconds(400));
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest1).first;
  pitEntry->insertOrUpdateInRecord(*this->face1, *interest1);
  pitEntry->insertOrUpdateOutRecord(*this->face3, *interest1);

  this->advanceClocks(time::milliseconds(300));
  shared_ptr<Interest> interest2 = makeInterest("/sIYw0TXWDj", 223);
  pitEntry->insertOrUpdateInRecord(*this->face1, *interest2);
  pitEntry->insertOrUpdateOutRecord(*this->face4, *interest2);

  this->advanceClocks(time::milliseconds(200)); // face3 has timed out

  lp::Nack nack4 = makeNack("/sIYw0TXWDj", 223, lp::NackReason::CONGESTION);
  pitEntry->getOutRecord(*this->face4)->setIncomingNack(nack4);
  this->strategy.afterReceiveNack(*this->face4, nack4, pitEntry);

  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.size(), 0);
}

// #3033 note-7
BOOST_FIXTURE_TEST_CASE_TEMPLATE(LiveDeadlock,
                                 S, Strategies, UnitTestTimeFixture)
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
    topo.setStrategy<S>(node);
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
  appA.expressInterest(Interest("/P/1"), nullptr, bind([&nNacksA]{ ++nNacksA; }), nullptr);
  appD.expressInterest(Interest("/P/1"), nullptr, bind([&nNacksD]{ ++nNacksD; }), nullptr);
  this->advanceClocks(time::milliseconds(1), time::milliseconds(5));
  appA.expressInterest(Interest("/P/1"), nullptr, bind([&nNacksA]{ ++nNacksA; }), nullptr);
  appD.expressInterest(Interest("/P/1"), nullptr, bind([&nNacksD]{ ++nNacksD; }), nullptr);
  this->advanceClocks(time::milliseconds(1), time::milliseconds(100));

  // As long as at least one Nack arrives at each client, strategy behavior is correct.
  // Whether both Interests are Nacked is a client face behavior, not strategy behavior.
  BOOST_CHECK_GT(nNacksA, 0);
  BOOST_CHECK_GT(nNacksD, 0);
}

template<lp::NackReason X, lp::NackReason Y, lp::NackReason R>
struct NackReasonCombination
{
  static lp::NackReason
  getX()
  {
    return X;
  }

  static lp::NackReason
  getY()
  {
    return Y;
  }

  static lp::NackReason
  getExpectedResult()
  {
    return R;
  }
};

using NackReasonCombinations = boost::mpl::vector<
  NackReasonCombination<lp::NackReason::CONGESTION, lp::NackReason::CONGESTION, lp::NackReason::CONGESTION>,
  NackReasonCombination<lp::NackReason::CONGESTION, lp::NackReason::DUPLICATE, lp::NackReason::CONGESTION>,
  NackReasonCombination<lp::NackReason::CONGESTION, lp::NackReason::NO_ROUTE, lp::NackReason::CONGESTION>,
  NackReasonCombination<lp::NackReason::CONGESTION, lp::NackReason::NONE, lp::NackReason::CONGESTION>,
  NackReasonCombination<lp::NackReason::DUPLICATE, lp::NackReason::CONGESTION, lp::NackReason::CONGESTION>,
  NackReasonCombination<lp::NackReason::DUPLICATE, lp::NackReason::DUPLICATE, lp::NackReason::DUPLICATE>,
  NackReasonCombination<lp::NackReason::DUPLICATE, lp::NackReason::NO_ROUTE, lp::NackReason::DUPLICATE>,
  NackReasonCombination<lp::NackReason::DUPLICATE, lp::NackReason::NONE, lp::NackReason::DUPLICATE>,
  NackReasonCombination<lp::NackReason::NO_ROUTE, lp::NackReason::CONGESTION, lp::NackReason::CONGESTION>,
  NackReasonCombination<lp::NackReason::NO_ROUTE, lp::NackReason::DUPLICATE, lp::NackReason::DUPLICATE>,
  NackReasonCombination<lp::NackReason::NO_ROUTE, lp::NackReason::NO_ROUTE, lp::NackReason::NO_ROUTE>,
  NackReasonCombination<lp::NackReason::NO_ROUTE, lp::NackReason::NONE, lp::NackReason::NO_ROUTE>,
  NackReasonCombination<lp::NackReason::NONE, lp::NackReason::CONGESTION, lp::NackReason::CONGESTION>,
  NackReasonCombination<lp::NackReason::NONE, lp::NackReason::DUPLICATE, lp::NackReason::DUPLICATE>,
  NackReasonCombination<lp::NackReason::NONE, lp::NackReason::NO_ROUTE, lp::NackReason::NO_ROUTE>,
  NackReasonCombination<lp::NackReason::NONE, lp::NackReason::NONE, lp::NackReason::NONE>
>;

// use BestRouteStrategy2 as a representative to test Nack reason combination
BOOST_FIXTURE_TEST_CASE_TEMPLATE(CombineReasons, Combination, NackReasonCombinations,
                                 StrategyNackReturnFixture<BestRouteStrategy2>)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fibEntry.addNextHop(*face3, 10);
  fibEntry.addNextHop(*face4, 20);
  fibEntry.addNextHop(*face5, 30);

  shared_ptr<Interest> interest1 = makeInterest("/F6sEwB24I", 282);
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest1).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest1);
  pitEntry->insertOrUpdateOutRecord(*face3, *interest1);
  pitEntry->insertOrUpdateOutRecord(*face4, *interest1);

  lp::Nack nack3 = makeNack(*interest1, Combination::getX());
  pitEntry->getOutRecord(*face3)->setIncomingNack(nack3);
  strategy.afterReceiveNack(*face3, nack3, pitEntry);

  BOOST_CHECK_EQUAL(strategy.sendNackHistory.size(), 0);

  lp::Nack nack4 = makeNack(*interest1, Combination::getY());
  pitEntry->getOutRecord(*face4)->setIncomingNack(nack4);
  strategy.afterReceiveNack(*face4, nack4, pitEntry);

  BOOST_REQUIRE_EQUAL(strategy.sendNackHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendNackHistory[0].header.getReason(), Combination::getExpectedResult());
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyNackReturn
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
