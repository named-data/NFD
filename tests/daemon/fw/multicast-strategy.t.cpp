/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
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
#include "choose-strategy.hpp"
#include "strategy-tester.hpp"
#include "topology-tester.hpp"

namespace nfd {
namespace fw {
namespace tests {

using MulticastStrategyTester = StrategyTester<MulticastStrategy>;
NFD_REGISTER_STRATEGY(MulticastStrategyTester);

class MulticastStrategyFixture : public GlobalIoTimeFixture
{
protected:
  MulticastStrategyFixture()
    : strategy(choose<MulticastStrategyTester>(forwarder))
    , face1(make_shared<DummyFace>())
    , face2(make_shared<DummyFace>())
    , face3(make_shared<DummyFace>())
  {
    faceTable.add(face1);
    faceTable.add(face2);
    faceTable.add(face3);
  }

  bool
  didSendInterestTo(const Face& face) const
  {
    auto it = std::find_if(strategy.sendInterestHistory.begin(),
                           strategy.sendInterestHistory.end(),
                           [&] (const auto& elem) { return elem.outFaceId == face.getId(); });
    return it != strategy.sendInterestHistory.end();
  }

protected:
  FaceTable faceTable;
  Forwarder forwarder{faceTable};
  MulticastStrategyTester& strategy;
  Fib& fib{forwarder.getFib()};
  Pit& pit{forwarder.getPit()};

  shared_ptr<DummyFace> face1;
  shared_ptr<DummyFace> face2;
  shared_ptr<DummyFace> face3;
};

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestMulticastStrategy, MulticastStrategyFixture)

BOOST_AUTO_TEST_CASE(Bug5123)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face2, 0);

  // Send an Interest from face 1 to face 2
  auto interest = makeInterest("ndn:/H0D6i5fc");
  auto pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest);

  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face1, 0), pitEntry);
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.size(), 1);

  // Advance more than default suppression
  this->advanceClocks(15_ms);

  // Get same interest from face 2 which does not have anywhere to go
  pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face2, *interest);

  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face2, 0), pitEntry);
  // Since the interest is the same as the one sent out earlier, the PIT entry should not be
  // rejected, as any data coming back must be able to satisfy the original interest from face 1
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory.size(), 0);

  /*
   *      +---------+            +---------+          +---------+
   *      |  nodeA  |------------|  nodeB  |----------|  nodeC  |
   *      +---------+    10ms    +---------+   100ms  +---------+
   */

  const Name PRODUCER_PREFIX = "/ndn/edu/nodeC/ping";

  TopologyTester topo;
  TopologyNode nodeA = topo.addForwarder("A"),
               nodeB = topo.addForwarder("B"),
               nodeC = topo.addForwarder("C");

  for (TopologyNode node : {nodeA, nodeB, nodeC}) {
    topo.setStrategy<MulticastStrategy>(node);
  }

  shared_ptr<TopologyLink> linkAB = topo.addLink("AB", 10_ms,  {nodeA, nodeB}),
                           linkBC = topo.addLink("BC", 100_ms, {nodeB, nodeC});

  shared_ptr<TopologyAppLink> appA = topo.addAppFace("cA", nodeA),
                              appB = topo.addAppFace("cB", nodeB),
                        pingServer = topo.addAppFace("p",  nodeC, PRODUCER_PREFIX);
  topo.addEchoProducer(pingServer->getClientFace());
  topo.registerPrefix(nodeA, linkAB->getFace(nodeA), PRODUCER_PREFIX, 10);
  topo.registerPrefix(nodeB, linkAB->getFace(nodeB), PRODUCER_PREFIX, 10);
  topo.registerPrefix(nodeB, linkBC->getFace(nodeB), PRODUCER_PREFIX, 100);

  Name name(PRODUCER_PREFIX);
  name.appendTimestamp();
  interest = makeInterest(name);
  appA->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);

  this->advanceClocks(10_ms, 20_ms);

  // AppB expresses the same interest
  interest->refreshNonce();
  appB->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(10_ms, 200_ms);

  // Data should have made to appB
  BOOST_CHECK_EQUAL(linkBC->getFace(nodeB).getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(linkAB->getFace(nodeA).getCounters().nInData, 0);

  this->advanceClocks(10_ms, 10_ms);
  // nodeA should have gotten the data successfully
  BOOST_CHECK_EQUAL(linkAB->getFace(nodeA).getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(topo.getForwarder(nodeA).getCounters().nUnsolicitedData, 0);
}

BOOST_AUTO_TEST_CASE(Forward2)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face1, 0);
  fib.addOrUpdateNextHop(fibEntry, *face2, 0);
  fib.addOrUpdateNextHop(fibEntry, *face3, 0);

  auto interest = makeInterest("ndn:/H0D6i5fc");
  auto pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face3, *interest);

  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face3, 0), pitEntry);
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.size(), 2);
  BOOST_TEST(didSendInterestTo(*face1));
  BOOST_TEST(didSendInterestTo(*face2));

  const auto TICK = time::duration_cast<time::nanoseconds>(MulticastStrategy::RETX_SUPPRESSION_INITIAL) / 10;

  // downstream retransmits frequently, but the strategy should not send Interests
  // more often than DEFAULT_MIN_RETX_INTERVAL
  scheduler::EventId retxFrom4Evt;
  size_t nSentLast = strategy.sendInterestHistory.size();
  auto timeSentLast = time::steady_clock::now();
  std::function<void()> periodicalRetxFrom4; // let periodicalRetxFrom4 lambda capture itself
  periodicalRetxFrom4 = [&] {
    pitEntry->insertOrUpdateInRecord(*face3, *interest);
    strategy.afterReceiveInterest(*interest, FaceEndpoint(*face3, 0), pitEntry);

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

BOOST_AUTO_TEST_CASE(LoopingInterest)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face1, 0);

  auto interest = makeInterest("ndn:/H0D6i5fc");
  auto pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest);

  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face1, 0), pitEntry);
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.size(), 0);
}

BOOST_AUTO_TEST_CASE(RetxSuppression)
{
  const auto suppressPeriod = MulticastStrategy::RETX_SUPPRESSION_INITIAL;
  BOOST_ASSERT(suppressPeriod >= 8_ms);

  // Set up the FIB
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face1, 0);
  fib.addOrUpdateNextHop(fibEntry, *face2, 0);
  fib.addOrUpdateNextHop(fibEntry, *face3, 0);

  // Interest arrives from face 1
  auto interest = makeInterest("/t8ZiSOi3");
  auto pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face1, 0), pitEntry);

  // forwarded to faces 2 and 3
  BOOST_TEST(strategy.sendInterestHistory.size() == 2);
  BOOST_TEST(didSendInterestTo(*face2));
  BOOST_TEST(didSendInterestTo(*face3));
  strategy.sendInterestHistory.clear();

  // still within the initial suppression period for face 2 and 3
  this->advanceClocks(suppressPeriod - 5_ms);

  // Interest arrives from face 2
  interest->refreshNonce();
  pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face2, *interest);
  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face2, 0), pitEntry);

  // forwarded only to face 1, suppressed on face 3
  BOOST_TEST(strategy.sendInterestHistory.size() == 1);
  BOOST_TEST(didSendInterestTo(*face1));
  strategy.sendInterestHistory.clear();

  // faces 2 and 3 no longer inside the suppression window
  this->advanceClocks(7_ms);

  // Interest arrives from face 3
  interest->refreshNonce();
  pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face3, *interest);
  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face3, 0), pitEntry);

  // suppressed on face 1, forwarded on face 2 (and suppression window doubles)
  BOOST_TEST(strategy.sendInterestHistory.size() == 1);
  BOOST_TEST(didSendInterestTo(*face2));
  strategy.sendInterestHistory.clear();

  // face 1 exits the suppression period, face 2 still inside
  this->advanceClocks(2 * suppressPeriod - 2_ms);

  // Interest arrives from face 3
  interest->refreshNonce();
  pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face3, *interest);
  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face3, 0), pitEntry);

  // forwarded only to face 1, suppressed on face 2
  BOOST_TEST(strategy.sendInterestHistory.size() == 1);
  BOOST_TEST(didSendInterestTo(*face1));
  strategy.sendInterestHistory.clear();

  // face 2 exits the suppression period
  this->advanceClocks(3_ms);

  // Interest arrives from face 1
  interest->refreshNonce();
  pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face1, 0), pitEntry);

  // forwarded to faces 2 and 3
  BOOST_TEST(strategy.sendInterestHistory.size() == 2);
  BOOST_TEST(didSendInterestTo(*face2));
  BOOST_TEST(didSendInterestTo(*face3));
  strategy.sendInterestHistory.clear();
}

BOOST_AUTO_TEST_CASE(NewNextHop)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face1, 0);
  fib.addOrUpdateNextHop(fibEntry, *face2, 0);

  auto interest = makeInterest("ndn:/H0D6i5fc");
  auto pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*face1, *interest);

  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face1, 0), pitEntry);
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.size(), 1);

  fib.addOrUpdateNextHop(fibEntry, *face3, 0);
  BOOST_CHECK_EQUAL(strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.size(), 2);
}

BOOST_AUTO_TEST_SUITE(LocalhopScope)

class ForwardAsyncFixture : public MulticastStrategyFixture
{
protected:
  shared_ptr<Face> inFace1;
  shared_ptr<Face> inFace2;
  shared_ptr<Face> fibFace1;
  shared_ptr<Face> fibFace2;
  shared_ptr<Face> newFibFace;

  size_t expectedInterests = 0;
};

class BasicNonLocal : public ForwardAsyncFixture
{
protected:
  BasicNonLocal()
  {
    inFace1 = face1;
    // inFace2 = nullptr;
    fibFace1 = face1;
    fibFace2 = face2;
    newFibFace = face3;
    expectedInterests = 0; // anything received on non-local face can only be sent to local face
  }
};

class NewFibLocal : public ForwardAsyncFixture
{
protected:
  NewFibLocal()
  {
    inFace1 = face1;
    // inFace2 = nullptr;
    fibFace1 = face1;
    fibFace2 = face2;
    newFibFace = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
    expectedInterests = 1;

    faceTable.add(newFibFace);
  }
};

class InFaceLocal : public ForwardAsyncFixture
{
protected:
  InFaceLocal()
  {
    inFace1 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
    // inFace2 = nullptr;
    fibFace1 = face1;
    fibFace2 = face2;
    newFibFace = face3;
    expectedInterests = 1;

    faceTable.add(inFace1);
  }
};

class InFaceLocalSameNewFace : public ForwardAsyncFixture
{
protected:
  InFaceLocalSameNewFace()
  {
    inFace1 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
    // inFace2 = nullptr;
    fibFace1 = face1;
    fibFace2 = face2;
    newFibFace = inFace1;
    expectedInterests = 0;

    faceTable.add(inFace1);
  }
};

class InFaceLocalAdHocSameNewFace : public ForwardAsyncFixture
{
protected:
  InFaceLocalAdHocSameNewFace()
  {
    inFace1 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL,
                                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                     ndn::nfd::LINK_TYPE_AD_HOC);
    // inFace2 = nullptr;
    fibFace1 = face1;
    fibFace2 = face2;
    newFibFace = inFace1;
    expectedInterests = 1;

    faceTable.add(inFace1);
  }
};

class InFaceLocalAndNonLocal1 : public ForwardAsyncFixture
{
protected:
  InFaceLocalAndNonLocal1()
  {
    inFace1 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
    inFace2 = face1;
    fibFace1 = face1;
    fibFace2 = face2;
    newFibFace = face3;
    expectedInterests = 1;

    faceTable.add(inFace1);
  }
};

class InFaceLocalAndNonLocal2 : public ForwardAsyncFixture
{
protected:
  InFaceLocalAndNonLocal2()
  {
    inFace1 = face1;
    inFace2 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
    fibFace1 = face1;
    fibFace2 = face2;
    newFibFace = face3;
    expectedInterests = 1;

    faceTable.add(inFace2);
  }
};

class InFaceSelection1 : public ForwardAsyncFixture
{
protected:
  InFaceSelection1()
  {
    inFace1 = face1;
    // inFace2 = nullptr;
    fibFace1 = face3;
    fibFace2 = face2;
    newFibFace = face1;

    expectedInterests = 0;
  }
};

class InFaceSelection2 : public ForwardAsyncFixture
{
protected:
  InFaceSelection2()
  {
    inFace1 = face2;
    inFace2 = face1;
    fibFace1 = face2;
    fibFace2 = face3;
    newFibFace = face1;

    // this test will trigger the check for additional branch, but it
    // still is not going to pass the localhop check
    expectedInterests = 0;
  }
};

using Tests = boost::mpl::vector<
  BasicNonLocal,
  NewFibLocal,
  InFaceLocal,
  InFaceLocalSameNewFace,
  InFaceLocalAdHocSameNewFace,
  InFaceLocalAndNonLocal1,
  InFaceLocalAndNonLocal2,
  InFaceSelection1,
  InFaceSelection2
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(ForwardAsync, T, Tests, T)
{
  fib::Entry& fibEntry = *this->fib.insert(Name("/localhop")).first;
  this->fib.addOrUpdateNextHop(fibEntry, *this->fibFace1, 0);
  this->fib.addOrUpdateNextHop(fibEntry, *this->fibFace2, 0);

  auto interest = makeInterest("ndn:/localhop/H0D6i5fc");
  auto pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->inFace1, *interest);
  this->strategy.afterReceiveInterest(*interest, FaceEndpoint(*this->inFace1, 0), pitEntry);

  if (this->inFace2 != nullptr) {
    auto interest2 = makeInterest("ndn:/localhop/H0D6i5fc");
    pitEntry->insertOrUpdateInRecord(*this->inFace2, *interest2);
    this->strategy.afterReceiveInterest(*interest2, FaceEndpoint(*this->inFace2, 0), pitEntry);
  }

  this->strategy.sendInterestHistory.clear();
  this->fib.addOrUpdateNextHop(fibEntry, *this->newFibFace, 0);
  BOOST_CHECK_EQUAL(this->strategy.sendInterestHistory.size(), this->expectedInterests);
}

BOOST_AUTO_TEST_SUITE_END() // LocalhopScope
BOOST_AUTO_TEST_SUITE_END() // TestMulticastStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
