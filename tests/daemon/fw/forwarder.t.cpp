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

#include "fw/forwarder.hpp"
#include "common/global.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "choose-strategy.hpp"
#include "dummy-strategy.hpp"

#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace tests {

class ForwarderFixture : public GlobalIoTimeFixture
{
protected:
  template<typename ...Args>
  shared_ptr<DummyFace>
  addFace(Args&&... args)
  {
    auto face = make_shared<DummyFace>(std::forward<Args>(args)...);
    faceTable.add(face);
    return face;
  }

protected:
  FaceTable faceTable;
  Forwarder forwarder{faceTable};
};

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestForwarder, ForwarderFixture)

BOOST_AUTO_TEST_CASE(SimpleExchange)
{
  auto face1 = addFace();
  auto face2 = addFace();

  Fib& fib = forwarder.getFib();
  fib::Entry* entry = fib.insert("/A").first;
  fib.addOrUpdateNextHop(*entry, *face2, 0);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nInInterests, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutInterests, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsHits, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsMisses, 0);
  face1->receiveInterest(*makeInterest("/A/B"), 0);
  this->advanceClocks(100_ms, 1_s);
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
  face2->receiveData(*makeData("/A/B"), 0);
  this->advanceClocks(100_ms, 1_s);
  BOOST_REQUIRE_EQUAL(face1->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentData[0].getName(), "/A/B");
  BOOST_REQUIRE(face1->sentData[0].getTag<lp::IncomingFaceIdTag>() != nullptr);
  BOOST_CHECK_EQUAL(*face1->sentData[0].getTag<lp::IncomingFaceIdTag>(), face2->getId());
  BOOST_CHECK_EQUAL(forwarder.getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutData, 1);
}

BOOST_AUTO_TEST_CASE(CsMatched)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace();

  Fib& fib = forwarder.getFib();
  fib::Entry* entry = fib.insert("/A").first;
  fib.addOrUpdateNextHop(*entry, *face2, 0);

  Pit& pit = forwarder.getPit();
  BOOST_CHECK_EQUAL(pit.size(), 0);

  auto data = makeData("/A/B");
  data->setTag(make_shared<lp::IncomingFaceIdTag>(face3->getId()));
  forwarder.getCs().insert(*data);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsHits, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsMisses, 0);
  face1->receiveInterest(*makeInterest("/A", true), 0);
  this->advanceClocks(1_ms, 5_ms);
  // Interest matching ContentStore should not be forwarded
  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsHits, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nCsMisses, 0);

  BOOST_REQUIRE_EQUAL(face1->sentData.size(), 1);
  // IncomingFaceId field should be reset to represent CS
  BOOST_CHECK_EQUAL(face1->sentData[0].getName(), "/A/B");
  BOOST_REQUIRE(face1->sentData[0].getTag<lp::IncomingFaceIdTag>() != nullptr);
  BOOST_CHECK_EQUAL(*face1->sentData[0].getTag<lp::IncomingFaceIdTag>(), face::FACEID_CONTENT_STORE);

  this->advanceClocks(100_ms, 500_ms);
  // PIT entry should not be left behind
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_CASE(OutgoingInterest)
{
  auto face1 = addFace();
  auto face2 = addFace();

  Pit& pit = forwarder.getPit();
  auto interestA1 = makeInterest("/A", false, nullopt, 8378);
  shared_ptr<pit::Entry> pitA = pit.insert(*interestA1).first;
  pitA->insertOrUpdateInRecord(*face1, *interestA1);

  auto interestA2 = makeInterest("/A", false, nullopt, 1698);
  forwarder.onOutgoingInterest(pitA, FaceEndpoint(*face2, 0), *interestA2);

  pit::OutRecordCollection::iterator outA2 = pitA->getOutRecord(*face2);
  BOOST_REQUIRE(outA2 != pitA->out_end());
  BOOST_CHECK_EQUAL(outA2->getLastNonce(), 1698);

  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentInterests.back().getNonce(), 1698);
}

BOOST_AUTO_TEST_CASE(NextHopFaceId)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace();

  Fib& fib = forwarder.getFib();
  fib::Entry* entry = fib.insert("/A").first;
  fib.addOrUpdateNextHop(*entry, *face3, 0);

  auto interest = makeInterest("/A/B");
  interest->setTag(make_shared<lp::NextHopFaceIdTag>(face2->getId()));

  face1->receiveInterest(*interest, 0);
  this->advanceClocks(100_ms, 1_s);
  BOOST_CHECK_EQUAL(face3->sentInterests.size(), 0);
  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentInterests.front().getName(), "/A/B");
}

class ScopeLocalhostIncomingTestForwarder : public Forwarder
{
public:
  using Forwarder::Forwarder;

  void
  onDataUnsolicited(const FaceEndpoint&, const Data&) final
  {
    ++onDataUnsolicited_count;
  }

protected:
  void
  dispatchToStrategy(pit::Entry&, std::function<void(fw::Strategy&)>) final
  {
    ++dispatchToStrategy_count;
  }

public:
  int dispatchToStrategy_count = 0;
  int onDataUnsolicited_count = 0;
};

BOOST_FIXTURE_TEST_CASE(ScopeLocalhostIncoming, GlobalIoTimeFixture)
{
  FaceTable faceTable;
  ScopeLocalhostIncomingTestForwarder forwarder(faceTable);

  auto face1 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  auto face2 = make_shared<DummyFace>();
  faceTable.add(face1);
  faceTable.add(face2);

  // local face, /localhost: OK
  forwarder.dispatchToStrategy_count = 0;
  auto i1 = makeInterest("/localhost/A1");
  forwarder.onIncomingInterest(FaceEndpoint(*face1, 0), *i1);
  BOOST_CHECK_EQUAL(forwarder.dispatchToStrategy_count, 1);

  // non-local face, /localhost: violate
  forwarder.dispatchToStrategy_count = 0;
  auto i2 = makeInterest("/localhost/A2");
  forwarder.onIncomingInterest(FaceEndpoint(*face2, 0), *i2);
  BOOST_CHECK_EQUAL(forwarder.dispatchToStrategy_count, 0);

  // local face, non-/localhost: OK
  forwarder.dispatchToStrategy_count = 0;
  auto i3 = makeInterest("/A3");
  forwarder.onIncomingInterest(FaceEndpoint(*face1, 0), *i3);
  BOOST_CHECK_EQUAL(forwarder.dispatchToStrategy_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.dispatchToStrategy_count = 0;
  auto i4 = makeInterest("/A4");
  forwarder.onIncomingInterest(FaceEndpoint(*face2, 0), *i4);
  BOOST_CHECK_EQUAL(forwarder.dispatchToStrategy_count, 1);

  // local face, /localhost: OK
  forwarder.onDataUnsolicited_count = 0;
  auto d1 = makeData("/localhost/B1");
  forwarder.onIncomingData(FaceEndpoint(*face1, 0), *d1);
  BOOST_CHECK_EQUAL(forwarder.onDataUnsolicited_count, 1);

  // non-local face, /localhost: OK
  forwarder.onDataUnsolicited_count = 0;
  auto d2 = makeData("/localhost/B2");
  forwarder.onIncomingData(FaceEndpoint(*face2, 0), *d2);
  BOOST_CHECK_EQUAL(forwarder.onDataUnsolicited_count, 0);

  // local face, non-/localhost: OK
  forwarder.onDataUnsolicited_count = 0;
  auto d3 = makeData("/B3");
  forwarder.onIncomingData(FaceEndpoint(*face1, 0), *d3);
  BOOST_CHECK_EQUAL(forwarder.onDataUnsolicited_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.onDataUnsolicited_count = 0;
  auto d4 = makeData("/B4");
  forwarder.onIncomingData(FaceEndpoint(*face2, 0), *d4);
  BOOST_CHECK_EQUAL(forwarder.onDataUnsolicited_count, 1);
}

BOOST_AUTO_TEST_CASE(IncomingInterestStrategyDispatch)
{
  auto face1 = addFace();
  auto face2 = addFace();

  DummyStrategy& strategyA = choose<DummyStrategy>(forwarder, "/", DummyStrategy::getStrategyName());
  DummyStrategy& strategyB = choose<DummyStrategy>(forwarder, "/B", DummyStrategy::getStrategyName());

  auto interest1 = makeInterest("/A/1");
  strategyA.afterReceiveInterest_count = 0;
  strategyA.interestOutFace = face2;
  forwarder.startProcessInterest(FaceEndpoint(*face1, 0), *interest1);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveInterest_count, 1);

  auto interest2 = makeInterest("/B/2", true);
  strategyB.afterReceiveInterest_count = 0;
  strategyB.interestOutFace = face2;
  forwarder.startProcessInterest(FaceEndpoint(*face1, 0), *interest2);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveInterest_count, 1);

  this->advanceClocks(1_ms, 5_ms);

  auto data1 = makeData("/A/1");
  strategyA.beforeSatisfyInterest_count = 0;
  forwarder.startProcessData(FaceEndpoint(*face2, 0), *data1);
  BOOST_CHECK_EQUAL(strategyA.beforeSatisfyInterest_count, 1);

  auto data2 = makeData("/B/2/b");
  strategyB.beforeSatisfyInterest_count = 0;
  forwarder.startProcessData(FaceEndpoint(*face2, 0), *data2);
  BOOST_CHECK_EQUAL(strategyB.beforeSatisfyInterest_count, 1);

  auto interest3 = makeInterest("/A/3", false, 30_ms);
  forwarder.startProcessInterest(FaceEndpoint(*face1, 0), *interest3);
  auto interest4 = makeInterest("/B/4", false, 5_s);
  forwarder.startProcessInterest(FaceEndpoint(*face1, 0), *interest4);
}

BOOST_AUTO_TEST_CASE(IncomingData)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace();
  auto face4 = addFace();

  Pit& pit = forwarder.getPit();
  auto interestD = makeInterest("/A/B/C/D");
  shared_ptr<pit::Entry> pitD = pit.insert(*interestD).first;
  pitD->insertOrUpdateInRecord(*face1, *interestD);
  auto interestA = makeInterest("/A", true);
  shared_ptr<pit::Entry> pitA = pit.insert(*interestA).first;
  pitA->insertOrUpdateInRecord(*face2, *interestA);
  pitA->insertOrUpdateInRecord(*face3, *interestA);
  auto interestC = makeInterest("/A/B/C", true);
  shared_ptr<pit::Entry> pitC = pit.insert(*interestC).first;
  pitC->insertOrUpdateInRecord(*face3, *interestC);
  pitC->insertOrUpdateInRecord(*face4, *interestC);

  auto dataD = makeData("/A/B/C/D");
  forwarder.onIncomingData(FaceEndpoint(*face3, 0), *dataD);
  this->advanceClocks(1_ms, 5_ms);

  BOOST_CHECK_EQUAL(face1->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face3->sentData.size(), 0);
  BOOST_CHECK_EQUAL(face4->sentData.size(), 1);
}

BOOST_AUTO_TEST_CASE(IncomingNack)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace("dummy://", "dummy://",
                       ndn::nfd::FACE_SCOPE_NON_LOCAL,
                       ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                       ndn::nfd::LINK_TYPE_MULTI_ACCESS);

  DummyStrategy& strategyA = choose<DummyStrategy>(forwarder, "/", DummyStrategy::getStrategyName());
  DummyStrategy& strategyB = choose<DummyStrategy>(forwarder, "/B", DummyStrategy::getStrategyName());

  Pit& pit = forwarder.getPit();

  // dispatch to the correct strategy
  auto interest1 = makeInterest("/A/AYJqayrzF", false, nullopt, 562);
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateOutRecord(*face1, *interest1);
  auto interest2 = makeInterest("/B/EVyP73ru", false, nullopt, 221);
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateOutRecord(*face1, *interest2);

  lp::Nack nack1 = makeNack(*interest1, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(FaceEndpoint(*face1, 0), nack1);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 1);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  lp::Nack nack2 = makeNack(*interest2, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(FaceEndpoint(*face1, 0), nack2);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 1);

  // record Nack on PIT out-record
  pit::OutRecordCollection::iterator outRecord1 = pit1->getOutRecord(*face1);
  BOOST_REQUIRE(outRecord1 != pit1->out_end());
  BOOST_REQUIRE(outRecord1->getIncomingNack() != nullptr);
  BOOST_CHECK_EQUAL(outRecord1->getIncomingNack()->getReason(), lp::NackReason::CONGESTION);

  // drop if no PIT entry
  lp::Nack nack3 = makeNack(*makeInterest("/yEcw5HhdM", false, nullopt, 243), lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(FaceEndpoint(*face1, 0), nack3);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if no out-record
  auto interest4 = makeInterest("/Etab4KpY", false, nullopt, 157);
  shared_ptr<pit::Entry> pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateOutRecord(*face1, *interest4);

  lp::Nack nack4a = makeNack(*interest4, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(FaceEndpoint(*face2, 0), nack4a);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if Nonce does not match out-record
  lp::Nack nack4b = makeNack(*makeInterest("/Etab4KpY", false, nullopt, 294), lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(FaceEndpoint(*face1, 0), nack4b);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if inFace is multi-access
  pit4->insertOrUpdateOutRecord(*face3, *interest4);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(FaceEndpoint(*face3, 0), nack4a);
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);
}

BOOST_AUTO_TEST_CASE(OutgoingNack)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace("dummy://", "dummy://",
                       ndn::nfd::FACE_SCOPE_NON_LOCAL,
                       ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                       ndn::nfd::LINK_TYPE_MULTI_ACCESS);

  Pit& pit = forwarder.getPit();

  lp::NackHeader nackHeader;
  nackHeader.setReason(lp::NackReason::CONGESTION);

  // don't send Nack if there's no in-record
  auto interest1 = makeInterest("/fM5IVEtC", false, nullopt, 719);
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(*face1, *interest1);

  face2->sentNacks.clear();
  forwarder.onOutgoingNack(pit1, FaceEndpoint(*face2, 0), nackHeader);
  BOOST_CHECK_EQUAL(face2->sentNacks.size(), 0);

  // send Nack with correct Nonce
  auto interest2a = makeInterest("/Vi8tRm9MG3", false, nullopt, 152);
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2a).first;
  pit2->insertOrUpdateInRecord(*face1, *interest2a);
  auto interest2b = makeInterest("/Vi8tRm9MG3", false, nullopt, 808);
  pit2->insertOrUpdateInRecord(*face2, *interest2b);

  face1->sentNacks.clear();
  forwarder.onOutgoingNack(pit2, FaceEndpoint(*face1, 0), nackHeader);
  BOOST_REQUIRE_EQUAL(face1->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentNacks.back().getReason(), lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(face1->sentNacks.back().getInterest().getNonce(), 152);

  // erase in-record
  pit::InRecordCollection::iterator inRecord2a = pit2->getInRecord(*face1);
  BOOST_CHECK(inRecord2a == pit2->in_end());

  // send Nack with correct Nonce
  face2->sentNacks.clear();
  forwarder.onOutgoingNack(pit2, FaceEndpoint(*face2, 0), nackHeader);
  BOOST_REQUIRE_EQUAL(face2->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getReason(), lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getInterest().getNonce(), 808);

  // erase in-record
  pit::InRecordCollection::iterator inRecord2b = pit2->getInRecord(*face1);
  BOOST_CHECK(inRecord2b == pit2->in_end());

  // don't send Nack to multi-access face
  auto interest2c = makeInterest("/Vi8tRm9MG3", false, nullopt, 228);
  pit2->insertOrUpdateInRecord(*face3, *interest2c);

  face3->sentNacks.clear();
  forwarder.onOutgoingNack(pit1, FaceEndpoint(*face3, 0), nackHeader);
  BOOST_CHECK_EQUAL(face3->sentNacks.size(), 0);
}

BOOST_AUTO_TEST_CASE(InterestLoopNack)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace("dummy://", "dummy://",
                       ndn::nfd::FACE_SCOPE_NON_LOCAL,
                       ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                       ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  auto face4 = addFace();

  Fib& fib = forwarder.getFib();
  fib::Entry* entry = fib.insert("/zT4XwK0Hnx").first;
  fib.addOrUpdateNextHop(*entry, *face4, 0);

  // receive Interest on face1
  face1->sentNacks.clear();
  auto interest1a = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", false, nullopt, 732);
  face1->receiveInterest(*interest1a, 0);
  BOOST_CHECK(face1->sentNacks.empty());

  // receive Interest with duplicate Nonce on face1: legit retransmission
  face1->sentNacks.clear();
  auto interest1b = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", false, nullopt, 732);
  face1->receiveInterest(*interest1b, 0);
  BOOST_CHECK(face1->sentNacks.empty());

  // receive Interest with duplicate Nonce on face2
  face2->sentNacks.clear();
  auto interest2a = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", false, nullopt, 732);
  face2->receiveInterest(*interest2a, 0);
  BOOST_REQUIRE_EQUAL(face2->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getInterest().wireEncode(), interest2a->wireEncode());
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getReason(), lp::NackReason::DUPLICATE);

  // receive Interest with new Nonce on face2
  face2->sentNacks.clear();
  auto interest2b = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", false, nullopt, 944);
  face2->receiveInterest(*interest2b, 0);
  BOOST_CHECK(face2->sentNacks.empty());

  // receive Interest with duplicate Nonce on face3, don't send Nack to multi-access face
  face3->sentNacks.clear();
  auto interest3a = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", false, nullopt, 732);
  face3->receiveInterest(*interest3a, 0);
  BOOST_CHECK(face3->sentNacks.empty());
}

BOOST_AUTO_TEST_CASE(InterestLoopWithShortLifetime) // Bug 1953
{
  auto face1 = addFace();
  auto face2 = addFace();

  // cause an Interest sent out of face2 to loop back into face1 after a delay
  face2->afterSend.connect([face1, face2] (uint32_t pktType) {
    if (pktType == tlv::Interest) {
      auto interest = make_shared<Interest>(face2->sentInterests.back());
      getScheduler().schedule(170_ms, [face1, interest] { face1->receiveInterest(*interest, 0); });
    }
  });

  Fib& fib = forwarder.getFib();
  fib::Entry* entry = fib.insert("/A").first;
  fib.addOrUpdateNextHop(*entry, *face2, 0);

  // receive an Interest
  auto interest = makeInterest("/A/1", false, 50_ms, 82101183);
  face1->receiveInterest(*interest, 0);

  // interest should be forwarded only once, as long as Nonce is in Dead Nonce List
  BOOST_ASSERT(25_ms * 40 < forwarder.getDeadNonceList().getLifetime());
  this->advanceClocks(25_ms, 40);

  BOOST_CHECK_EQUAL(face2->sentInterests.size(), 1);

  // It's unnecessary to check that Interest with duplicate Nonce can be forwarded again
  // after it's gone from Dead Nonce List, because the entry lifetime of Dead Nonce List
  // is an implementation decision. NDN protocol requires Name+Nonce to be unique,
  // without specifying when Name+Nonce could repeat. Forwarder is permitted to suppress
  // an Interest if its Name+Nonce has appeared any point in the past.
}

BOOST_AUTO_TEST_CASE(PitLeak) // Bug 3484
{
  auto face1 = addFace();

  auto interest = makeInterest("/hcLSAsQ9A", false, 2_s, 61883075);

  DeadNonceList& dnl = forwarder.getDeadNonceList();
  dnl.add(interest->getName(), interest->getNonce());
  Pit& pit = forwarder.getPit();
  BOOST_REQUIRE_EQUAL(pit.size(), 0);

  forwarder.startProcessInterest(FaceEndpoint(*face1, 0), *interest);
  this->advanceClocks(100_ms, 20_s);
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestForwarder
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace nfd
