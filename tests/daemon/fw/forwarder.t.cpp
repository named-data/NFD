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
  BOOST_CHECK_EQUAL(forwarder.getCounters().nSatisfiedInterests, 0);

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
  BOOST_CHECK_EQUAL(forwarder.getCounters().nInNacks, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutNacks, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nSatisfiedInterests, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 0);
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

BOOST_AUTO_TEST_CASE(InterestWithoutNonce)
{
  auto face1 = addFace();
  auto face2 = addFace();

  Fib& fib = forwarder.getFib();
  fib::Entry* entry = fib.insert("/A").first;
  fib.addOrUpdateNextHop(*entry, *face2, 0);

  auto interest = makeInterest("/A");
  BOOST_CHECK_EQUAL(interest->hasNonce(), false);
  face1->receiveInterest(*interest, 0);

  // Ensure Nonce added if incoming packet did not have Nonce
  BOOST_REQUIRE_EQUAL(face2->getCounters().nOutInterests, 1);
  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentInterests.back().hasNonce(), true);
}

BOOST_AUTO_TEST_CASE(OutgoingInterest)
{
  auto face1 = addFace();
  auto face2 = addFace();

  Pit& pit = forwarder.getPit();
  auto interestA1 = makeInterest("/A", false, nullopt, 8378);
  auto pitA = pit.insert(*interestA1).first;
  pitA->insertOrUpdateInRecord(*face1, *interestA1);

  auto interestA2 = makeInterest("/A", false, nullopt, 1698);
  auto outA2 = forwarder.onOutgoingInterest(*interestA2, *face2, pitA);
  BOOST_REQUIRE(outA2 != nullptr);
  BOOST_CHECK_EQUAL(outA2->getLastNonce(), 1698);

  // This packet will be dropped because HopLimit=0
  auto interestA3 = makeInterest("/A", false, nullopt, 9876);
  interestA3->setHopLimit(0);
  auto outA3 = forwarder.onOutgoingInterest(*interestA3, *face2, pitA);
  BOOST_CHECK(outA3 == nullptr);

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

BOOST_AUTO_TEST_CASE(HopLimit)
{
  auto faceIn = addFace();
  auto faceRemote = addFace("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_NON_LOCAL);
  auto faceLocal = addFace("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  Fib& fib = forwarder.getFib();
  fib::Entry* entryRemote = fib.insert("/remote").first;
  fib.addOrUpdateNextHop(*entryRemote, *faceRemote, 0);
  fib::Entry* entryLocal = fib.insert("/local").first;
  fib.addOrUpdateNextHop(*entryLocal, *faceLocal, 0);

  // Incoming interest w/o HopLimit will not be dropped on send or receive paths
  auto interestNoHopLimit = makeInterest("/remote/abcdefgh");
  faceIn->receiveInterest(*interestNoHopLimit, 0);
  this->advanceClocks(100_ms, 1_s);
  BOOST_CHECK_EQUAL(faceRemote->sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nInHopLimitZero, 0);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nOutHopLimitZero, 0);
  BOOST_REQUIRE_EQUAL(faceRemote->sentInterests.size(), 1);
  BOOST_CHECK(!faceRemote->sentInterests.back().getHopLimit());

  // Incoming interest w/ HopLimit > 1 will not be dropped on send/receive
  auto interestHopLimit2 = makeInterest("/remote/ijklmnop");
  interestHopLimit2->setHopLimit(2);
  faceIn->receiveInterest(*interestHopLimit2, 0);
  this->advanceClocks(100_ms, 1_s);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nOutInterests, 2);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nInHopLimitZero, 0);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nOutHopLimitZero, 0);
  BOOST_REQUIRE_EQUAL(faceRemote->sentInterests.size(), 2);
  BOOST_REQUIRE(faceRemote->sentInterests.back().getHopLimit());
  BOOST_CHECK_EQUAL(*faceRemote->sentInterests.back().getHopLimit(), 1);

  // Incoming interest w/ HopLimit == 1 will be dropped on send path if going out on remote face
  auto interestHopLimit1Remote = makeInterest("/remote/qrstuvwx");
  interestHopLimit1Remote->setHopLimit(1);
  faceIn->receiveInterest(*interestHopLimit1Remote, 0);
  this->advanceClocks(100_ms, 1_s);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nOutInterests, 2);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nInHopLimitZero, 0);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nOutHopLimitZero, 1);
  BOOST_CHECK_EQUAL(faceRemote->sentInterests.size(), 2);

  // Incoming interest w/ HopLimit == 1 will not be dropped on send path if going out on local face
  auto interestHopLimit1Local = makeInterest("/local/abcdefgh");
  interestHopLimit1Local->setHopLimit(1);
  faceIn->receiveInterest(*interestHopLimit1Local, 0);
  this->advanceClocks(100_ms, 1_s);
  BOOST_CHECK_EQUAL(faceLocal->getCounters().nOutInterests, 1);
  BOOST_CHECK_EQUAL(faceLocal->getCounters().nInHopLimitZero, 0);
  BOOST_CHECK_EQUAL(faceLocal->getCounters().nOutHopLimitZero, 0);
  BOOST_REQUIRE_EQUAL(faceLocal->sentInterests.size(), 1);
  BOOST_REQUIRE(faceLocal->sentInterests.back().getHopLimit());
  BOOST_CHECK_EQUAL(*faceLocal->sentInterests.back().getHopLimit(), 0);

  // Interest w/ HopLimit == 0 will be dropped on receive path
  auto interestHopLimit0 = makeInterest("/remote/yzabcdef");
  interestHopLimit0->setHopLimit(0);
  faceIn->receiveInterest(*interestHopLimit0, 0);
  this->advanceClocks(100_ms, 1_s);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nOutInterests, 2);
  BOOST_CHECK_EQUAL(faceIn->getCounters().nInHopLimitZero, 1);
  BOOST_CHECK_EQUAL(faceRemote->getCounters().nOutHopLimitZero, 1);
  BOOST_CHECK_EQUAL(faceRemote->sentInterests.size(), 2);
}

BOOST_AUTO_TEST_CASE(AddDefaultHopLimit)
{
  auto face = addFace();
  auto faceEndpoint = FaceEndpoint(*face, 0);
  Pit& pit = forwarder.getPit();
  auto i1 = makeInterest("/A");
  auto pitA = pit.insert(*i1).first;

  // By default, no HopLimit should be added
  auto i2 = makeInterest("/A");
  BOOST_TEST(!i2->getHopLimit().has_value());
  forwarder.onContentStoreMiss(*i2, faceEndpoint, pitA);
  BOOST_TEST(!i2->getHopLimit().has_value());

  // Change config value to 10
  forwarder.m_config.defaultHopLimit = 10;

  // HopLimit should be set to 10 now
  auto i3 = makeInterest("/A");
  BOOST_TEST(!i3->getHopLimit().has_value());
  forwarder.onContentStoreMiss(*i3, faceEndpoint, pitA);
  BOOST_REQUIRE(i3->getHopLimit().has_value());
  BOOST_TEST(*i3->getHopLimit() == 10);

  // An existing HopLimit should be preserved
  auto i4 = makeInterest("/A");
  i4->setHopLimit(50);
  forwarder.onContentStoreMiss(*i4, faceEndpoint, pitA);
  BOOST_REQUIRE(i4->getHopLimit().has_value());
  BOOST_TEST(*i4->getHopLimit() == 50);
}

BOOST_AUTO_TEST_CASE(ScopeLocalhostIncoming)
{
  auto face1 = addFace("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  auto face2 = addFace(); // default is non-local

  auto& strategy = choose<DummyStrategy>(forwarder, "/", DummyStrategy::getStrategyName());

  // local face, /localhost: OK
  strategy.afterReceiveInterest_count = 0;
  auto i1 = makeInterest("/localhost/A1");
  forwarder.onIncomingInterest(*i1, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(strategy.afterReceiveInterest_count, 1);

  // non-local face, /localhost: violate
  strategy.afterReceiveInterest_count = 0;
  auto i2 = makeInterest("/localhost/A2");
  forwarder.onIncomingInterest(*i2, FaceEndpoint(*face2, 0));
  BOOST_CHECK_EQUAL(strategy.afterReceiveInterest_count, 0);

  // local face, non-/localhost: OK
  strategy.afterReceiveInterest_count = 0;
  auto i3 = makeInterest("/A3");
  forwarder.onIncomingInterest(*i3, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(strategy.afterReceiveInterest_count, 1);

  // non-local face, non-/localhost: OK
  strategy.afterReceiveInterest_count = 0;
  auto i4 = makeInterest("/A4");
  forwarder.onIncomingInterest(*i4, FaceEndpoint(*face2, 0));
  BOOST_CHECK_EQUAL(strategy.afterReceiveInterest_count, 1);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 0);

  // local face, /localhost: OK
  auto d1 = makeData("/localhost/B1");
  forwarder.onIncomingData(*d1, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 1);

  // non-local face, /localhost: violate
  auto d2 = makeData("/localhost/B2");
  forwarder.onIncomingData(*d2, FaceEndpoint(*face2, 0));
  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 1);

  // local face, non-/localhost: OK
  auto d3 = makeData("/B3");
  forwarder.onIncomingData(*d3, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 2);

  // non-local face, non-/localhost: OK
  auto d4 = makeData("/B4");
  forwarder.onIncomingData(*d4, FaceEndpoint(*face2, 0));
  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 3);
}

BOOST_AUTO_TEST_CASE(IncomingInterestStrategyDispatch)
{
  auto face1 = addFace();
  auto face2 = addFace();

  auto& strategyA = choose<DummyStrategy>(forwarder, "/", DummyStrategy::getStrategyName());
  auto& strategyB = choose<DummyStrategy>(forwarder, "/B", DummyStrategy::getStrategyName());

  auto interest1 = makeInterest("/A/1");
  strategyA.afterReceiveInterest_count = 0;
  strategyA.interestOutFace = face2;
  forwarder.onIncomingInterest(*interest1, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(strategyA.afterReceiveInterest_count, 1);

  auto interest2 = makeInterest("/B/2", true);
  strategyB.afterReceiveInterest_count = 0;
  strategyB.interestOutFace = face2;
  forwarder.onIncomingInterest(*interest2, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(strategyB.afterReceiveInterest_count, 1);

  this->advanceClocks(1_ms, 5_ms);

  auto data1 = makeData("/A/1");
  strategyA.beforeSatisfyInterest_count = 0;
  forwarder.onIncomingData(*data1, FaceEndpoint(*face2, 0));
  BOOST_CHECK_EQUAL(strategyA.beforeSatisfyInterest_count, 1);

  auto data2 = makeData("/B/2/b");
  strategyB.beforeSatisfyInterest_count = 0;
  forwarder.onIncomingData(*data2, FaceEndpoint(*face2, 0));
  BOOST_CHECK_EQUAL(strategyB.beforeSatisfyInterest_count, 1);

  auto interest3 = makeInterest("/A/3", false, 30_ms);
  forwarder.onIncomingInterest(*interest3, FaceEndpoint(*face1, 0));
  auto interest4 = makeInterest("/B/4", false, 5_s);
  forwarder.onIncomingInterest(*interest4, FaceEndpoint(*face1, 0));
}

BOOST_AUTO_TEST_CASE(IncomingData)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace();
  auto face4 = addFace();

  Pit& pit = forwarder.getPit();
  auto interestD = makeInterest("/A/B/C/D");
  auto pitD = pit.insert(*interestD).first;
  pitD->insertOrUpdateInRecord(*face1, *interestD);
  auto interestA = makeInterest("/A", true);
  auto pitA = pit.insert(*interestA).first;
  pitA->insertOrUpdateInRecord(*face2, *interestA);
  pitA->insertOrUpdateInRecord(*face3, *interestA);
  auto interestC = makeInterest("/A/B/C", true);
  auto pitC = pit.insert(*interestC).first;
  pitC->insertOrUpdateInRecord(*face3, *interestC);
  pitC->insertOrUpdateInRecord(*face4, *interestC);

  auto dataD = makeData("/A/B/C/D");
  forwarder.onIncomingData(*dataD, FaceEndpoint(*face3, 0));
  this->advanceClocks(1_ms, 5_ms);

  BOOST_CHECK_EQUAL(face1->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face3->sentData.size(), 0);
  BOOST_CHECK_EQUAL(face4->sentData.size(), 1);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutData, 3);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 0);
}

BOOST_AUTO_TEST_CASE(OutgoingData)
{
  auto face1 = addFace("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  auto face2 = addFace("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_NON_LOCAL);
  auto face3 = addFace();
  face3->setId(face::INVALID_FACEID);

  auto data = makeData("/QkzAWU6K");
  auto localData = makeData("/localhost/YH8bqnbv");

  face1->sentData.clear();
  BOOST_CHECK(forwarder.onOutgoingData(*data, *face1));
  BOOST_REQUIRE_EQUAL(face1->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentData.back().getName(), data->getName());

  // scope control
  face1->sentData.clear();
  face2->sentData.clear();
  BOOST_CHECK(!forwarder.onOutgoingData(*localData, *face2));
  BOOST_CHECK_EQUAL(face2->sentData.size(), 0);
  BOOST_CHECK(forwarder.onOutgoingData(*localData, *face1));
  BOOST_REQUIRE_EQUAL(face1->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentData.back().getName(), localData->getName());

  // face with invalid ID
  face3->sentData.clear();
  BOOST_CHECK(!forwarder.onOutgoingData(*data, *face3));
  BOOST_CHECK_EQUAL(face3->sentData.size(), 0);
}

BOOST_AUTO_TEST_CASE(IncomingNack)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace("dummy://", "dummy://",
                       ndn::nfd::FACE_SCOPE_NON_LOCAL,
                       ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                       ndn::nfd::LINK_TYPE_MULTI_ACCESS);

  auto& strategyA = choose<DummyStrategy>(forwarder, "/", DummyStrategy::getStrategyName());
  auto& strategyB = choose<DummyStrategy>(forwarder, "/B", DummyStrategy::getStrategyName());

  Pit& pit = forwarder.getPit();

  // dispatch to the correct strategy
  auto interest1 = makeInterest("/A/AYJqayrzF", false, nullopt, 562);
  auto pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateOutRecord(*face1, *interest1);
  auto interest2 = makeInterest("/B/EVyP73ru", false, nullopt, 221);
  auto pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateOutRecord(*face1, *interest2);

  auto nack1 = makeNack(*interest1, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(nack1, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 1);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  auto nack2 = makeNack(*interest2, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(nack2, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 1);

  // record Nack on PIT out-record
  auto outRecord1 = pit1->getOutRecord(*face1);
  BOOST_REQUIRE(outRecord1 != pit1->out_end());
  BOOST_REQUIRE(outRecord1->getIncomingNack() != nullptr);
  BOOST_CHECK_EQUAL(outRecord1->getIncomingNack()->getReason(), lp::NackReason::CONGESTION);

  // drop if no PIT entry
  auto nack3 = makeNack(*makeInterest("/yEcw5HhdM", false, nullopt, 243), lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(nack3, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if no out-record
  auto interest4 = makeInterest("/Etab4KpY", false, nullopt, 157);
  auto pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateOutRecord(*face1, *interest4);

  auto nack4a = makeNack(*interest4, lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(nack4a, FaceEndpoint(*face2, 0));
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if Nonce does not match out-record
  auto nack4b = makeNack(*makeInterest("/Etab4KpY", false, nullopt, 294), lp::NackReason::CONGESTION);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(nack4b, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(strategyA.afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyB.afterReceiveNack_count, 0);

  // drop if inFace is multi-access
  pit4->insertOrUpdateOutRecord(*face3, *interest4);
  strategyA.afterReceiveNack_count = 0;
  strategyB.afterReceiveNack_count = 0;
  forwarder.onIncomingNack(nack4a, FaceEndpoint(*face3, 0));
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
  auto face4 = addFace();
  face4->setId(face::INVALID_FACEID);

  Pit& pit = forwarder.getPit();

  lp::NackHeader nackHeader;
  nackHeader.setReason(lp::NackReason::CONGESTION);

  // don't send Nack if there's no in-record
  auto interest1 = makeInterest("/fM5IVEtC", false, nullopt, 719);
  auto pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(*face1, *interest1);

  face2->sentNacks.clear();
  BOOST_CHECK(!forwarder.onOutgoingNack(nackHeader, *face2, pit1));
  BOOST_CHECK_EQUAL(face2->sentNacks.size(), 0);

  // send Nack with correct Nonce
  auto interest2a = makeInterest("/Vi8tRm9MG3", false, nullopt, 152);
  auto pit2 = pit.insert(*interest2a).first;
  pit2->insertOrUpdateInRecord(*face1, *interest2a);
  auto interest2b = makeInterest("/Vi8tRm9MG3", false, nullopt, 808);
  pit2->insertOrUpdateInRecord(*face2, *interest2b);
  face1->sentNacks.clear();
  face2->sentNacks.clear();

  BOOST_CHECK(forwarder.onOutgoingNack(nackHeader, *face1, pit2));
  BOOST_REQUIRE_EQUAL(face1->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentNacks.back().getReason(), lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(face1->sentNacks.back().getInterest().getNonce(), 152);
  BOOST_CHECK_EQUAL(face2->sentNacks.size(), 0);

  // in-record is erased
  auto inRecord2a = pit2->getInRecord(*face1);
  BOOST_CHECK(inRecord2a == pit2->in_end());

  // send Nack with correct Nonce
  BOOST_CHECK(forwarder.onOutgoingNack(nackHeader, *face2, pit2));
  BOOST_CHECK_EQUAL(face1->sentNacks.size(), 1);
  BOOST_REQUIRE_EQUAL(face2->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getReason(), lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(face2->sentNacks.back().getInterest().getNonce(), 808);

  // in-record is erased
  auto inRecord2b = pit2->getInRecord(*face2);
  BOOST_CHECK(inRecord2b == pit2->in_end());

  // don't send Nack to multi-access face
  auto interest2c = makeInterest("/Vi8tRm9MG3", false, nullopt, 228);
  pit2->insertOrUpdateInRecord(*face3, *interest2c);

  face3->sentNacks.clear();
  BOOST_CHECK(!forwarder.onOutgoingNack(nackHeader, *face3, pit2));
  BOOST_CHECK_EQUAL(face3->sentNacks.size(), 0);

  // don't send Nack to face with invalid ID
  auto interest1b = makeInterest("/fM5IVEtC", false, nullopt, 553);
  pit1->insertOrUpdateInRecord(*face4, *interest1b);

  face4->sentNacks.clear();
  BOOST_CHECK(!forwarder.onOutgoingNack(nackHeader, *face4, pit1));
  BOOST_CHECK_EQUAL(face4->sentNacks.size(), 0);
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

  auto& dnl = forwarder.getDeadNonceList();
  dnl.add(interest->getName(), interest->getNonce());
  auto& pit = forwarder.getPit();
  BOOST_CHECK_EQUAL(pit.size(), 0);

  forwarder.onIncomingInterest(*interest, FaceEndpoint(*face1, 0));
  this->advanceClocks(100_ms, 20_s);
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_CASE(UnsolicitedData)
{
  auto face1 = addFace();
  auto data = makeData("/A");

  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 0);
  forwarder.onIncomingData(*data, FaceEndpoint(*face1, 0));
  this->advanceClocks(1_ms, 10_ms);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nUnsolicitedData, 1);
}

BOOST_AUTO_TEST_CASE(NewNextHop)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace();
  auto face4 = addFace();

  auto& strategy = choose<DummyStrategy>(forwarder, "/A", DummyStrategy::getStrategyName());

  Fib& fib = forwarder.getFib();
  Pit& pit = forwarder.getPit();

  // fib: "/", "/A/B", "/A/B/C/D/E"
  fib::Entry* entry = fib.insert("/").first;
  fib.addOrUpdateNextHop(*entry, *face1, 100);
  entry = fib.insert("/A/B").first;
  fib.addOrUpdateNextHop(*entry, *face2, 0);
  entry = fib.insert("/A/B/C/D/E").first;
  fib.addOrUpdateNextHop(*entry, *face3, 0);

  // pit: "/A", "/A/B/C", "/A/B/Z"
  auto interest1 = makeInterest("/A");
  auto pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(*face3, *interest1);
  auto interest2 = makeInterest("/A/B/C");
  auto pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateInRecord(*face3, *interest2);
  auto interest3 = makeInterest("/A/B/Z");
  auto pit3 = pit.insert(*interest3).first;
  pit3->insertOrUpdateInRecord(*face3, *interest3);

  // new nexthop for "/"
  entry = fib.insert("/").first;
  fib.addOrUpdateNextHop(*entry, *face2, 50);

  // /A     --> triggered
  // /A/B/C --> not triggered
  // /A/B/Z --> not triggered
  BOOST_TEST_REQUIRE(strategy.afterNewNextHopCalls.size() == 1);
  BOOST_TEST(strategy.afterNewNextHopCalls[0] == "/A");
  strategy.afterNewNextHopCalls.clear();

  // new nexthop for "/A"
  entry = fib.insert("/A").first;
  fib.addOrUpdateNextHop(*entry, *face4, 50);

  // /A     --> triggered
  // /A/B/C --> not triggered
  // /A/B/Z --> not triggered
  BOOST_TEST_REQUIRE(strategy.afterNewNextHopCalls.size() == 1);
  BOOST_TEST(strategy.afterNewNextHopCalls[0] == "/A");
  strategy.afterNewNextHopCalls.clear();

  // new nexthop for "/A/B"
  entry = fib.insert("/A/B").first;
  fib.addOrUpdateNextHop(*entry, *face4, 0);

  // /A     --> not triggered
  // /A/B/C --> triggered
  // /A/B/Z --> triggered
  BOOST_TEST_REQUIRE(strategy.afterNewNextHopCalls.size() == 2);
  BOOST_TEST(strategy.afterNewNextHopCalls[0] == "/A/B/C");
  BOOST_TEST(strategy.afterNewNextHopCalls[1] == "/A/B/Z");
  strategy.afterNewNextHopCalls.clear();

  // new nexthop for "/A/B/C/D"
  entry = fib.insert("/A/B/C/D").first;
  fib.addOrUpdateNextHop(*entry, *face1, 0);

  // nothing triggered
  BOOST_TEST(strategy.afterNewNextHopCalls.size() == 0);

  // create a second pit entry for /A
  auto interest4 = makeInterest("/A");
  interest4->setMustBeFresh(true);
  auto pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateInRecord(*face3, *interest4);

  // new nexthop for "/A"
  entry = fib.insert("/A").first;
  fib.addOrUpdateNextHop(*entry, *face1, 0);

  // /A     --> triggered twice
  // /A/B/C --> not triggered
  // /A/B/Z --> not triggered
  BOOST_TEST_REQUIRE(strategy.afterNewNextHopCalls.size() == 2);
  BOOST_TEST(strategy.afterNewNextHopCalls[0] == "/A");
  BOOST_TEST(strategy.afterNewNextHopCalls[1] == "/A");
}

BOOST_AUTO_TEST_SUITE(ProcessConfig)

BOOST_AUTO_TEST_CASE(DefaultHopLimit)
{
  ConfigFile cf;
  forwarder.setConfigFile(cf);

  std::string config = R"CONFIG(
    forwarder
    {
      default_hop_limit 10
    }
  )CONFIG";

  // The default value is 0
  BOOST_TEST(forwarder.m_config.defaultHopLimit == 0);

  // Dry run parsing should not change the default config
  cf.parse(config, true, "dummy-config");
  BOOST_TEST(forwarder.m_config.defaultHopLimit == 0);

  // Check if the actual parsing works
  cf.parse(config, false, "dummy-config");
  BOOST_TEST(forwarder.m_config.defaultHopLimit == 10);

  // After removing default_hop_limit from the config file,
  // the default value of zero should be restored
  config = R"CONFIG(
    forwarder
    {
    }
  )CONFIG";

  cf.parse(config, false, "dummy-config");
  BOOST_TEST(forwarder.m_config.defaultHopLimit == 0);
}

BOOST_AUTO_TEST_CASE(BadDefaultHopLimit)
{
  ConfigFile cf;
  forwarder.setConfigFile(cf);

  // not a number
  std::string config = R"CONFIG(
    forwarder
    {
      default_hop_limit hello
    }
  )CONFIG";

  BOOST_CHECK_THROW(cf.parse(config, true, "dummy-config"), ConfigFile::Error);
  BOOST_CHECK_THROW(cf.parse(config, false, "dummy-config"), ConfigFile::Error);

  // negative number
  config = R"CONFIG(
    forwarder
    {
      default_hop_limit -1
    }
  )CONFIG";

  BOOST_CHECK_THROW(cf.parse(config, true, "dummy-config"), ConfigFile::Error);
  BOOST_CHECK_THROW(cf.parse(config, false, "dummy-config"), ConfigFile::Error);

  // out of range
  config = R"CONFIG(
    forwarder
    {
      default_hop_limit 256
    }
  )CONFIG";

  BOOST_CHECK_THROW(cf.parse(config, true, "dummy-config"), ConfigFile::Error);
  BOOST_CHECK_THROW(cf.parse(config, false, "dummy-config"), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

BOOST_AUTO_TEST_SUITE_END() // TestForwarder
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace nfd
