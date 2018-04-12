/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "choose-strategy.hpp"
#include "dummy-strategy.hpp"

#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestForwarder, UnitTestTimeFixture)

BOOST_AUTO_TEST_CASE(SimpleExchange)
{
  Forwarder forwarder;

  shared_ptr<Interest> interestAB = makeInterest("/A/B");
  interestAB->setInterestLifetime(time::seconds(4));
  shared_ptr<Data> dataABC = makeData("/A/B/C");

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  fib.insert("/A").first->addNextHop(*face2, 0);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nInInterests, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutInterests, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsHits, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsMisses, 0);
  face1->receiveInterest(*interestAB);
  this->advanceClocks(time::milliseconds(100), time::seconds(1));
  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentInterests[0].getName(), "/A/B");
  BOOST_REQUIRE(face2->sentInterests[0].getTag<lp::IncomingFaceIdTag>() != nullptr);
  BOOST_CHECK_EQUAL(*face2->sentInterests[0].getTag<lp::IncomingFaceIdTag>(), face1->getId());
  BOOST_CHECK_EQUAL(forwarder.getCounters().nInInterests, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutInterests, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsHits, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsMisses, 1);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nInData, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutData, 0);
  face2->receiveData(*dataABC);
  this->advanceClocks(time::milliseconds(100), time::seconds(1));
  BOOST_REQUIRE_EQUAL(face1->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentData[0].getName(), "/A/B/C");
  BOOST_REQUIRE(face1->sentData[0].getTag<lp::IncomingFaceIdTag>() != nullptr);
  BOOST_CHECK_EQUAL(*face1->sentData[0].getTag<lp::IncomingFaceIdTag>(), face2->getId());
  BOOST_CHECK_EQUAL(forwarder.getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutData, 1);
}

BOOST_AUTO_TEST_CASE(CsMatched)
{
  Forwarder forwarder;

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  shared_ptr<Interest> interestA = makeInterest("/A");
  interestA->setInterestLifetime(time::seconds(4));
  shared_ptr<Data> dataA = makeData("/A");
  dataA->setTag(make_shared<lp::IncomingFaceIdTag>(face3->getId()));

  Fib& fib = forwarder.getFib();
  fib.insert("/A").first->addNextHop(*face2, 0);

  Pit& pit = forwarder.getPit();
  BOOST_CHECK_EQUAL(pit.size(), 0);

  Cs& cs = forwarder.getCs();
  cs.insert(*dataA);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsHits, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsMisses, 0);
  face1->receiveInterest(*interestA);
  this->advanceClocks(time::milliseconds(1), time::milliseconds(5));
  // Interest matching ContentStore should not be forwarded
  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsHits, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsMisses, 0);

  BOOST_REQUIRE_EQUAL(face1->sentData.size(), 1);
  // IncomingFaceId field should be reset to represent CS
  BOOST_REQUIRE(face1->sentData[0].getTag<lp::IncomingFaceIdTag>() != nullptr);
  BOOST_CHECK_EQUAL(*face1->sentData[0].getTag<lp::IncomingFaceIdTag>(), face::FACEID_CONTENT_STORE);

  this->advanceClocks(time::milliseconds(100), time::milliseconds(500));
  // PIT entry should not be left behind
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_CASE(OutgoingInterest)
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Pit& pit = forwarder.getPit();
  auto interestA1 = makeInterest("/A");
  interestA1->setNonce(8378);
  shared_ptr<pit::Entry> pitA = pit.insert(*interestA1).first;
  pitA->insertOrUpdateInRecord(*face1, *interestA1);

  auto interestA2 = makeInterest("/A");
  interestA2->setNonce(1698);
  forwarder.onOutgoingInterest(pitA, *face2, *interestA2);

  pit::OutRecordCollection::iterator outA2 = pitA->getOutRecord(*face2);
  BOOST_REQUIRE(outA2 != pitA->out_end());
  BOOST_CHECK_EQUAL(outA2->getLastNonce(), 1698);

  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentInterests.back().getNonce(), 1698);
}

BOOST_AUTO_TEST_CASE(NextHopFaceId)
{
  Forwarder forwarder;

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  fib.insert("/A").first->addNextHop(*face3, 0);

  shared_ptr<Interest> interest = makeInterest("/A/B");
  interest->setTag(make_shared<lp::NextHopFaceIdTag>(face2->getId()));

  face1->receiveInterest(*interest);
  this->advanceClocks(time::milliseconds(100), time::seconds(1));
  BOOST_CHECK_EQUAL(face3->sentInterests.size(), 0);
  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentInterests.front().getName(), "/A/B");
}

class ScopeLocalhostIncomingTestForwarder : public Forwarder
{
public:
  ScopeLocalhostIncomingTestForwarder()
  {
  }

  void
  onDataUnsolicited(Face& inFace, const Data& data) override
  {
    ++onDataUnsolicited_count;
  }

protected:
  void
  dispatchToStrategy(pit::Entry&, std::function<void(fw::Strategy&)>) override
  {
    ++dispatchToStrategy_count;
  }

public:
  int dispatchToStrategy_count;
  int onDataUnsolicited_count;
};

BOOST_AUTO_TEST_CASE(ScopeLocalhostIncoming)
{
  ScopeLocalhostIncomingTestForwarder forwarder;
  auto face1 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  auto face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  // local face, /localhost: OK
  forwarder.dispatchToStrategy_count = 0;
  shared_ptr<Interest> i1 = makeInterest("/localhost/A1");
  forwarder.onIncomingInterest(*face1, *i1);
  BOOST_CHECK_EQUAL(forwarder.dispatchToStrategy_count, 1);

  // non-local face, /localhost: violate
  forwarder.dispatchToStrategy_count = 0;
  shared_ptr<Interest> i2 = makeInterest("/localhost/A2");
  forwarder.onIncomingInterest(*face2, *i2);
  BOOST_CHECK_EQUAL(forwarder.dispatchToStrategy_count, 0);

  // local face, non-/localhost: OK
  forwarder.dispatchToStrategy_count = 0;
  shared_ptr<Interest> i3 = makeInterest("/A3");
  forwarder.onIncomingInterest(*face1, *i3);
  BOOST_CHECK_EQUAL(forwarder.dispatchToStrategy_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.dispatchToStrategy_count = 0;
  shared_ptr<Interest> i4 = makeInterest("/A4");
  forwarder.onIncomingInterest(*face2, *i4);
  BOOST_CHECK_EQUAL(forwarder.dispatchToStrategy_count, 1);

  // local face, /localhost: OK
  forwarder.onDataUnsolicited_count = 0;
  shared_ptr<Data> d1 = makeData("/localhost/B1");
  forwarder.onIncomingData(*face1, *d1);
  BOOST_CHECK_EQUAL(forwarder.onDataUnsolicited_count, 1);

  // non-local face, /localhost: OK
  forwarder.onDataUnsolicited_count = 0;
  shared_ptr<Data> d2 = makeData("/localhost/B2");
  forwarder.onIncomingData(*face2, *d2);
  BOOST_CHECK_EQUAL(forwarder.onDataUnsolicited_count, 0);

  // local face, non-/localhost: OK
  forwarder.onDataUnsolicited_count = 0;
  shared_ptr<Data> d3 = makeData("/B3");
  forwarder.onIncomingData(*face1, *d3);
  BOOST_CHECK_EQUAL(forwarder.onDataUnsolicited_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.onDataUnsolicited_count = 0;
  shared_ptr<Data> d4 = makeData("/B4");
  forwarder.onIncomingData(*face2, *d4);
  BOOST_CHECK_EQUAL(forwarder.onDataUnsolicited_count, 1);
}

BOOST_AUTO_TEST_CASE(IncomingInterestStrategyDispatch)
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  DummyStrategy& strategyA = choose<DummyStrategy>(forwarder, "ndn:/", DummyStrategy::getStrategyName());
  DummyStrategy& strategyB = choose<DummyStrategy>(forwarder, "ndn:/B", DummyStrategy::getStrategyName());

  shared_ptr<Interest> interest1 = makeInterest("ndn:/A/1");
  strategyA.afterReceiveInterest_count = 0;
  strategyA.interestOutFace = face2;
  forwarder.startProcessInterest(*face1, *interest1);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveInterest_count, 1);

  shared_ptr<Interest> interest2 = makeInterest("ndn:/B/2");
  strategyB.afterReceiveInterest_count = 0;
  strategyB.interestOutFace = face2;
  forwarder.startProcessInterest(*face1, *interest2);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveInterest_count, 1);

  this->advanceClocks(time::milliseconds(1), time::milliseconds(5));

  shared_ptr<Data> data1 = makeData("ndn:/A/1/a");
  strategyA.beforeSatisfyInterest_count = 0;
  forwarder.startProcessData(*face2, *data1);
  BOOST_CHECK_EQUAL(strategyA.beforeSatisfyInterest_count, 1);

  shared_ptr<Data> data2 = makeData("ndn:/B/2/b");
  strategyB.beforeSatisfyInterest_count = 0;
  forwarder.startProcessData(*face2, *data2);
  BOOST_CHECK_EQUAL(strategyB.beforeSatisfyInterest_count, 1);

  shared_ptr<Interest> interest3 = makeInterest("ndn:/A/3");
  interest3->setInterestLifetime(time::milliseconds(30));
  forwarder.startProcessInterest(*face1, *interest3);
  shared_ptr<Interest> interest4 = makeInterest("ndn:/B/4");
  interest4->setInterestLifetime(time::milliseconds(5000));
  forwarder.startProcessInterest(*face1, *interest4);
}

BOOST_AUTO_TEST_CASE(IncomingData)
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>();
  auto face4 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  forwarder.addFace(face4);

  Pit& pit = forwarder.getPit();
  shared_ptr<Interest> interest0 = makeInterest("ndn:/");
  shared_ptr<pit::Entry> pit0 = pit.insert(*interest0).first;
  pit0->insertOrUpdateInRecord(*face1, *interest0);
  shared_ptr<Interest> interestA = makeInterest("ndn:/A");
  shared_ptr<pit::Entry> pitA = pit.insert(*interestA).first;
  pitA->insertOrUpdateInRecord(*face1, *interestA);
  pitA->insertOrUpdateInRecord(*face2, *interestA);
  shared_ptr<Interest> interestC = makeInterest("ndn:/A/B/C");
  shared_ptr<pit::Entry> pitC = pit.insert(*interestC).first;
  pitC->insertOrUpdateInRecord(*face3, *interestC);
  pitC->insertOrUpdateInRecord(*face4, *interestC);

  shared_ptr<Data> dataD = makeData("ndn:/A/B/C/D");
  forwarder.onIncomingData(*face3, *dataD);
  this->advanceClocks(time::milliseconds(1), time::milliseconds(5));

  BOOST_CHECK_EQUAL(face1->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face3->sentData.size(), 0);
  BOOST_CHECK_EQUAL(face4->sentData.size(), 1);
}

BOOST_AUTO_TEST_CASE(IncomingNack)
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>("dummy://", "dummy://",
                                      ndn::nfd::FACE_SCOPE_NON_LOCAL,
                                      ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                      ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  DummyStrategy& strategyA = choose<DummyStrategy>(forwarder, "ndn:/", DummyStrategy::getStrategyName());
  DummyStrategy& strategyB = choose<DummyStrategy>(forwarder, "ndn:/B", DummyStrategy::getStrategyName());

  Pit& pit = forwarder.getPit();

  // dispatch to the correct strategy
  shared_ptr<Interest> interest1 = makeInterest("/A/AYJqayrzF", 562);
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateOutRecord(*face1, *interest1);
  shared_ptr<Interest> interest2 = makeInterest("/B/EVyP73ru", 221);
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateOutRecord(*face1, *interest2);

  lp::Nack nack1 = makeNack("/A/AYJqayrzF", 562, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face1, nack1);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 1);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  lp::Nack nack2 = makeNack("/B/EVyP73ru", 221, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face1, nack2);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 1);

  // record Nack on PIT out-record
  pit::OutRecordCollection::iterator outRecord1 = pit1->getOutRecord(*face1);
  BOOST_REQUIRE(outRecord1 != pit1->out_end());
  BOOST_REQUIRE(outRecord1->getIncomingNack() != nullptr);
  BOOST_CHECK_EQUAL(outRecord1->getIncomingNack()->getReason(), lp::NackReason::CONGESTION);

  // drop if no PIT entry
  lp::Nack nack3 = makeNack("/yEcw5HhdM", 243, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face1, nack3);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if no out-record
  shared_ptr<Interest> interest4 = makeInterest("/Etab4KpY", 157);
  shared_ptr<pit::Entry> pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateOutRecord(*face1, *interest4);

  lp::Nack nack4a = makeNack("/Etab4KpY", 157, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face2, nack4a);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if Nonce does not match out-record
  lp::Nack nack4b = makeNack("/Etab4KpY", 294, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face1, nack4b);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if inFace is multi-access
  pit4->insertOrUpdateOutRecord(*face3, *interest4);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face3, nack4a);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);
}

BOOST_AUTO_TEST_CASE(OutgoingNack)
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>("dummy://", "dummy://",
                                      ndn::nfd::FACE_SCOPE_NON_LOCAL,
                                      ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                      ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Pit& pit = forwarder.getPit();

  lp::NackHeader nackHeader;
  nackHeader.setReason(lp::NackReason::CONGESTION);

  // don't send Nack if there's no in-record
  shared_ptr<Interest> interest1 = makeInterest("/fM5IVEtC", 719);
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(*face1, *interest1);

  face2->sentNacks.clear();
  forwarder.onOutgoingNack(pit1, *face2, nackHeader);
  BOOST_CHECK_EQUAL(face2->sentNacks.size(), 0);

  // send Nack with correct Nonce
  shared_ptr<Interest> interest2a = makeInterest("/Vi8tRm9MG3", 152);
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2a).first;
  pit2->insertOrUpdateInRecord(*face1, *interest2a);
  shared_ptr<Interest> interest2b = makeInterest("/Vi8tRm9MG3", 808);
  pit2->insertOrUpdateInRecord(*face2, *interest2b);

  face1->sentNacks.clear();
  forwarder.onOutgoingNack(pit2, *face1, nackHeader);
  BOOST_REQUIRE_EQUAL(face1->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentNacks.back().getReason(), lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(face1->sentNacks.back().getInterest().getNonce(), 152);

  // erase in-record
  pit::InRecordCollection::iterator inRecord2a = pit2->getInRecord(*face1);
  BOOST_CHECK(inRecord2a == pit2->in_end());

  // send Nack with correct Nonce
  face2->sentNacks.clear();
  forwarder.onOutgoingNack(pit2, *face2, nackHeader);
  BOOST_REQUIRE_EQUAL(face2->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getReason(), lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getInterest().getNonce(), 808);

  // erase in-record
  pit::InRecordCollection::iterator inRecord2b = pit2->getInRecord(*face1);
  BOOST_CHECK(inRecord2b == pit2->in_end());

  // don't send Nack to multi-access face
  shared_ptr<Interest> interest2c = makeInterest("/Vi8tRm9MG3", 228);
  pit2->insertOrUpdateInRecord(*face3, *interest2c);

  face3->sentNacks.clear();
  forwarder.onOutgoingNack(pit1, *face3, nackHeader);
  BOOST_CHECK_EQUAL(face3->sentNacks.size(), 0);
}

BOOST_AUTO_TEST_CASE(InterestLoopNack)
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>("dummy://", "dummy://",
                                      ndn::nfd::FACE_SCOPE_NON_LOCAL,
                                      ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                      ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  auto face4 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  forwarder.addFace(face4);

  Fib& fib = forwarder.getFib();
  fib.insert("/zT4XwK0Hnx").first->addNextHop(*face4, 0);

  // receive Interest on face1
  face1->sentNacks.clear();
  shared_ptr<Interest> interest1a = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", 732);
  face1->receiveInterest(*interest1a);
  BOOST_CHECK(face1->sentNacks.empty());

  // receive Interest with duplicate Nonce on face1: legit retransmission
  face1->sentNacks.clear();
  shared_ptr<Interest> interest1b = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", 732);
  face1->receiveInterest(*interest1b);
  BOOST_CHECK(face1->sentNacks.empty());

  // receive Interest with duplicate Nonce on face2
  face2->sentNacks.clear();
  shared_ptr<Interest> interest2a = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", 732);
  face2->receiveInterest(*interest2a);
  BOOST_REQUIRE_EQUAL(face2->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getInterest(), *interest2a);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getReason(), lp::NackReason::DUPLICATE);

  // receive Interest with new Nonce on face2
  face2->sentNacks.clear();
  shared_ptr<Interest> interest2b = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", 944);
  face2->receiveInterest(*interest2b);
  BOOST_CHECK(face2->sentNacks.empty());

  // receive Interest with duplicate Nonce on face3, don't send Nack to multi-access face
  face3->sentNacks.clear();
  shared_ptr<Interest> interest3a = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", 732);
  face3->receiveInterest(*interest3a);
  BOOST_CHECK(face3->sentNacks.empty());
}

BOOST_FIXTURE_TEST_CASE(InterestLoopWithShortLifetime, UnitTestTimeFixture) // Bug 1953
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  // cause an Interest sent out of face2 to loop back into face1 after a delay
  face2->afterSend.connect([face1, face2] (uint32_t pktType) {
    if (pktType == tlv::Interest) {
      auto interest = make_shared<Interest>(face2->sentInterests.back());
      scheduler::schedule(time::milliseconds(170), [face1, interest] { face1->receiveInterest(*interest); });
    }
  });

  Fib& fib = forwarder.getFib();
  fib.insert("/A").first->addNextHop(*face2, 0);

  // receive an Interest
  shared_ptr<Interest> interest = makeInterest("ndn:/A/1");
  interest->setNonce(82101183);
  interest->setInterestLifetime(time::milliseconds(50));
  face1->receiveInterest(*interest);

  // interest should be forwarded only once, as long as Nonce is in Dead Nonce List
  BOOST_ASSERT(time::milliseconds(25) * 40 < forwarder.getDeadNonceList().getLifetime());
  this->advanceClocks(time::milliseconds(25), 40);

  BOOST_CHECK_EQUAL(face2->sentInterests.size(), 1);

  // It's unnecessary to check that Interest with duplicate Nonce can be forwarded again
  // after it's gone from Dead Nonce List, because the entry lifetime of Dead Nonce List
  // is an implementation decision. NDN protocol requires Name+Nonce to be unique,
  // without specifying when Name+Nonce could repeat. Forwarder is permitted to suppress
  // an Interest if its Name+Nonce has appeared any point in the past.
}

BOOST_AUTO_TEST_CASE(PitLeak) // Bug 3484
{
  Forwarder forwarder;
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  forwarder.addFace(face1);

  shared_ptr<Interest> interest = makeInterest("ndn:/hcLSAsQ9A");
  interest->setNonce(61883075);
  interest->setInterestLifetime(time::seconds(2));

  DeadNonceList& dnl = forwarder.getDeadNonceList();
  dnl.add(interest->getName(), interest->getNonce());
  Pit& pit = forwarder.getPit();
  BOOST_REQUIRE_EQUAL(pit.size(), 0);

  forwarder.startProcessInterest(*face1, *interest);
  this->advanceClocks(time::milliseconds(100), time::seconds(20));
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
