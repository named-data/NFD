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

#include "fw/forwarder.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "dummy-strategy.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestForwarder, BaseFixture)

BOOST_AUTO_TEST_CASE(SimpleExchange)
{
  LimitedIo limitedIo;
  auto afterOp = bind(&LimitedIo::afterOp, &limitedIo);;
  Forwarder forwarder;

  Name nameA  ("ndn:/A");
  Name nameAB ("ndn:/A/B");
  Name nameABC("ndn:/A/B/C");
  shared_ptr<Interest> interestAB = makeInterest(nameAB);
  interestAB->setInterestLifetime(time::seconds(4));
  shared_ptr<Data> dataABC = makeData(nameABC);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  face1->afterSend.connect(bind(afterOp));
  face2->afterSend.connect(bind(afterOp));
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name("ndn:/A")).first;
  fibEntry->addNextHop(face2, 0);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nInInterests, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutInterests, 0);
  g_io.post([&] { face1->receiveInterest(*interestAB); });
  BOOST_CHECK_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face2->sentInterests[0].getName(), nameAB);
  BOOST_REQUIRE(face2->sentInterests[0].getTag<lp::IncomingFaceIdTag>() != nullptr);
  BOOST_CHECK_EQUAL(*face2->sentInterests[0].getTag<lp::IncomingFaceIdTag>(), face1->getId());
  BOOST_CHECK_EQUAL(forwarder.getCounters().nInInterests, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutInterests, 1);

  BOOST_CHECK_EQUAL(forwarder.getCounters().nInData, 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutData, 0);
  g_io.post([&] { face2->receiveData(*dataABC); });
  BOOST_CHECK_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
  BOOST_REQUIRE_EQUAL(face1->sentData.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentData[0].getName(), nameABC);
  BOOST_REQUIRE(face1->sentData[0].getTag<lp::IncomingFaceIdTag>() != nullptr);
  BOOST_CHECK_EQUAL(*face1->sentData[0].getTag<lp::IncomingFaceIdTag>(), face2->getId());
  BOOST_CHECK_EQUAL(forwarder.getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().nOutData, 1);
}

BOOST_AUTO_TEST_CASE(CsMatched)
{
  LimitedIo limitedIo;
  Forwarder forwarder;

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  shared_ptr<Interest> interestA = makeInterest("ndn:/A");
  interestA->setInterestLifetime(time::seconds(4));
  shared_ptr<Data> dataA = makeData("ndn:/A");
  dataA->setTag(make_shared<lp::IncomingFaceIdTag>(face3->getId()));

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name("ndn:/A")).first;
  fibEntry->addNextHop(face2, 0);

  Pit& pit = forwarder.getPit();
  BOOST_CHECK_EQUAL(pit.size(), 0);

  Cs& cs = forwarder.getCs();
  cs.insert(*dataA);

  face1->receiveInterest(*interestA);
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(5));
  // Interest matching ContentStore should not be forwarded
  BOOST_REQUIRE_EQUAL(face2->sentInterests.size(), 0);

  BOOST_REQUIRE_EQUAL(face1->sentData.size(), 1);
  // IncomingFaceId field should be reset to represent CS
  BOOST_REQUIRE(face1->sentData[0].getTag<lp::IncomingFaceIdTag>() != nullptr);
  BOOST_CHECK_EQUAL(*face1->sentData[0].getTag<lp::IncomingFaceIdTag>(), face::FACEID_CONTENT_STORE);

  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(500));
  // PIT entry should not be left behind
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

class ScopeLocalhostIncomingTestForwarder : public Forwarder
{
public:
  ScopeLocalhostIncomingTestForwarder()
  {
  }

  virtual void
  onDataUnsolicited(Face& inFace, const Data& data) DECL_OVERRIDE
  {
    ++onDataUnsolicited_count;
  }

protected:
  virtual void
  dispatchToStrategy(shared_ptr<pit::Entry> pitEntry, function<void(fw::Strategy*)> f) DECL_OVERRIDE
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

BOOST_AUTO_TEST_CASE(ScopeLocalhostOutgoing)
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  Pit& pit = forwarder.getPit();

  // local face, /localhost: OK
  shared_ptr<Interest> interestA1 = makeInterest("/localhost/A1");
  shared_ptr<pit::Entry> pitA1 = pit.insert(*interestA1).first;
  pitA1->insertOrUpdateInRecord(face3, *interestA1);
  face1->sentInterests.clear();
  forwarder.onOutgoingInterest(pitA1, *face1);
  BOOST_CHECK_EQUAL(face1->sentInterests.size(), 1);

  // non-local face, /localhost: violate
  shared_ptr<Interest> interestA2 = makeInterest("/localhost/A2");
  shared_ptr<pit::Entry> pitA2 = pit.insert(*interestA2).first;
  pitA2->insertOrUpdateInRecord(face3, *interestA2);
  face2->sentInterests.clear();
  forwarder.onOutgoingInterest(pitA2, *face2);
  BOOST_CHECK_EQUAL(face2->sentInterests.size(), 0);

  // local face, non-/localhost: OK
  shared_ptr<Interest> interestA3 = makeInterest("/A3");
  shared_ptr<pit::Entry> pitA3 = pit.insert(*interestA3).first;
  pitA3->insertOrUpdateInRecord(face3, *interestA3);
  face1->sentInterests.clear();
  forwarder.onOutgoingInterest(pitA3, *face1);
  BOOST_CHECK_EQUAL(face1->sentInterests.size(), 1);

  // non-local face, non-/localhost: OK
  shared_ptr<Interest> interestA4 = makeInterest("/A4");
  shared_ptr<pit::Entry> pitA4 = pit.insert(*interestA4).first;
  pitA4->insertOrUpdateInRecord(face3, *interestA4);
  face2->sentInterests.clear();
  forwarder.onOutgoingInterest(pitA4, *face2);
  BOOST_CHECK_EQUAL(face2->sentInterests.size(), 1);

  // local face, /localhost: OK
  face1->sentData.clear();
  forwarder.onOutgoingData(Data("/localhost/B1"), *face1);
  BOOST_CHECK_EQUAL(face1->sentData.size(), 1);

  // non-local face, /localhost: OK
  face2->sentData.clear();
  forwarder.onOutgoingData(Data("/localhost/B2"), *face2);
  BOOST_CHECK_EQUAL(face2->sentData.size(), 0);

  // local face, non-/localhost: OK
  face1->sentData.clear();
  forwarder.onOutgoingData(Data("/B3"), *face1);
  BOOST_CHECK_EQUAL(face1->sentData.size(), 1);

  // non-local face, non-/localhost: OK
  face2->sentData.clear();
  forwarder.onOutgoingData(Data("/B4"), *face2);
  BOOST_CHECK_EQUAL(face2->sentData.size(), 1);
}

BOOST_AUTO_TEST_CASE(ScopeLocalhopOutgoing)
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  auto face4 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  forwarder.addFace(face4);
  Pit& pit = forwarder.getPit();

  // from local face, to local face: OK
  shared_ptr<Interest> interest1 = makeInterest("/localhop/1");
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(face1, *interest1);
  face3->sentInterests.clear();
  forwarder.onOutgoingInterest(pit1, *face3);
  BOOST_CHECK_EQUAL(face3->sentInterests.size(), 1);

  // from non-local face, to local face: OK
  shared_ptr<Interest> interest2 = makeInterest("/localhop/2");
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateInRecord(face2, *interest2);
  face3->sentInterests.clear();
  forwarder.onOutgoingInterest(pit2, *face3);
  BOOST_CHECK_EQUAL(face3->sentInterests.size(), 1);

  // from local face, to non-local face: OK
  shared_ptr<Interest> interest3 = makeInterest("/localhop/3");
  shared_ptr<pit::Entry> pit3 = pit.insert(*interest3).first;
  pit3->insertOrUpdateInRecord(face1, *interest3);
  face4->sentInterests.clear();
  forwarder.onOutgoingInterest(pit3, *face4);
  BOOST_CHECK_EQUAL(face4->sentInterests.size(), 1);

  // from non-local face, to non-local face: violate
  shared_ptr<Interest> interest4 = makeInterest("/localhop/4");
  shared_ptr<pit::Entry> pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateInRecord(face2, *interest4);
  face4->sentInterests.clear();
  forwarder.onOutgoingInterest(pit4, *face4);
  BOOST_CHECK_EQUAL(face4->sentInterests.size(), 0);

  // from local face and non-local face, to local face: OK
  shared_ptr<Interest> interest5 = makeInterest("/localhop/5");
  shared_ptr<pit::Entry> pit5 = pit.insert(*interest5).first;
  pit5->insertOrUpdateInRecord(face1, *interest5);
  pit5->insertOrUpdateInRecord(face2, *interest5);
  face3->sentInterests.clear();
  forwarder.onOutgoingInterest(pit5, *face3);
  BOOST_CHECK_EQUAL(face3->sentInterests.size(), 1);

  // from local face and non-local face, to non-local face: OK
  shared_ptr<Interest> interest6 = makeInterest("/localhop/6");
  shared_ptr<pit::Entry> pit6 = pit.insert(*interest6).first;
  pit6->insertOrUpdateInRecord(face1, *interest6);
  pit6->insertOrUpdateInRecord(face2, *interest6);
  face4->sentInterests.clear();
  forwarder.onOutgoingInterest(pit6, *face4);
  BOOST_CHECK_EQUAL(face4->sentInterests.size(), 1);
}

BOOST_AUTO_TEST_CASE(IncomingInterestStrategyDispatch)
{
  LimitedIo limitedIo;
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  shared_ptr<DummyStrategy> strategyP = make_shared<DummyStrategy>(
                                        ref(forwarder), "ndn:/strategyP");
  shared_ptr<DummyStrategy> strategyQ = make_shared<DummyStrategy>(
                                        ref(forwarder), "ndn:/strategyQ");
  strategyChoice.install(strategyP);
  strategyChoice.install(strategyQ);
  strategyChoice.insert("ndn:/" , strategyP->getName());
  strategyChoice.insert("ndn:/B", strategyQ->getName());

  shared_ptr<Interest> interest1 = makeInterest("ndn:/A/1");
  strategyP->afterReceiveInterest_count = 0;
  strategyP->interestOutFace = face2;
  forwarder.startProcessInterest(*face1, *interest1);
  BOOST_CHECK_EQUAL(strategyP->afterReceiveInterest_count, 1);

  shared_ptr<Interest> interest2 = makeInterest("ndn:/B/2");
  strategyQ->afterReceiveInterest_count = 0;
  strategyQ->interestOutFace = face2;
  forwarder.startProcessInterest(*face1, *interest2);
  BOOST_CHECK_EQUAL(strategyQ->afterReceiveInterest_count, 1);

  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(5));

  shared_ptr<Data> data1 = makeData("ndn:/A/1/a");
  strategyP->beforeSatisfyInterest_count = 0;
  forwarder.startProcessData(*face2, *data1);
  BOOST_CHECK_EQUAL(strategyP->beforeSatisfyInterest_count, 1);

  shared_ptr<Data> data2 = makeData("ndn:/B/2/b");
  strategyQ->beforeSatisfyInterest_count = 0;
  forwarder.startProcessData(*face2, *data2);
  BOOST_CHECK_EQUAL(strategyQ->beforeSatisfyInterest_count, 1);

  shared_ptr<Interest> interest3 = makeInterest("ndn:/A/3");
  interest3->setInterestLifetime(time::milliseconds(30));
  forwarder.startProcessInterest(*face1, *interest3);
  shared_ptr<Interest> interest4 = makeInterest("ndn:/B/4");
  interest4->setInterestLifetime(time::milliseconds(5000));
  forwarder.startProcessInterest(*face1, *interest4);

  strategyP->beforeExpirePendingInterest_count = 0;
  strategyQ->beforeExpirePendingInterest_count = 0;
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(100));
  BOOST_CHECK_EQUAL(strategyP->beforeExpirePendingInterest_count, 1);
  BOOST_CHECK_EQUAL(strategyQ->beforeExpirePendingInterest_count, 0);
}

BOOST_AUTO_TEST_CASE(IncomingData)
{
  LimitedIo limitedIo;
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
  pit0->insertOrUpdateInRecord(face1, *interest0);
  shared_ptr<Interest> interestA = makeInterest("ndn:/A");
  shared_ptr<pit::Entry> pitA = pit.insert(*interestA).first;
  pitA->insertOrUpdateInRecord(face1, *interestA);
  pitA->insertOrUpdateInRecord(face2, *interestA);
  shared_ptr<Interest> interestC = makeInterest("ndn:/A/B/C");
  shared_ptr<pit::Entry> pitC = pit.insert(*interestC).first;
  pitC->insertOrUpdateInRecord(face3, *interestC);
  pitC->insertOrUpdateInRecord(face4, *interestC);

  shared_ptr<Data> dataD = makeData("ndn:/A/B/C/D");
  forwarder.onIncomingData(*face3, *dataD);
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(5));

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

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  shared_ptr<DummyStrategy> strategyP = make_shared<DummyStrategy>(
                                        ref(forwarder), "ndn:/strategyP");
  shared_ptr<DummyStrategy> strategyQ = make_shared<DummyStrategy>(
                                        ref(forwarder), "ndn:/strategyQ");
  strategyChoice.install(strategyP);
  strategyChoice.install(strategyQ);
  strategyChoice.insert("ndn:/" , strategyP->getName());
  strategyChoice.insert("ndn:/B", strategyQ->getName());

  Pit& pit = forwarder.getPit();

  // dispatch to the correct strategy
  shared_ptr<Interest> interest1 = makeInterest("/A/AYJqayrzF", 562);
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateOutRecord(face1, *interest1);
  shared_ptr<Interest> interest2 = makeInterest("/B/EVyP73ru", 221);
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateOutRecord(face1, *interest2);

  lp::Nack nack1 = makeNack("/A/AYJqayrzF", 562, lp::NackReason::CONGESTION);
  strategyP->afterReceiveNack_count = 0;
  strategyQ->afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face1, nack1);
  BOOST_CHECK_EQUAL(strategyP->afterReceiveNack_count, 1);
  BOOST_CHECK_EQUAL(strategyQ->afterReceiveNack_count, 0);

  lp::Nack nack2 = makeNack("/B/EVyP73ru", 221, lp::NackReason::CONGESTION);
  strategyP->afterReceiveNack_count = 0;
  strategyQ->afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face1, nack2);
  BOOST_CHECK_EQUAL(strategyP->afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyQ->afterReceiveNack_count, 1);

  // record Nack on PIT out-record
  pit::OutRecordCollection::iterator outRecord1 = pit1->getOutRecord(*face1);
  BOOST_REQUIRE(outRecord1 != pit1->out_end());
  BOOST_REQUIRE(outRecord1->getIncomingNack() != nullptr);
  BOOST_CHECK_EQUAL(outRecord1->getIncomingNack()->getReason(), lp::NackReason::CONGESTION);

  // drop if no PIT entry
  lp::Nack nack3 = makeNack("/yEcw5HhdM", 243, lp::NackReason::CONGESTION);
  strategyP->afterReceiveNack_count = 0;
  strategyQ->afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face1, nack3);
  BOOST_CHECK_EQUAL(strategyP->afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyQ->afterReceiveNack_count, 0);

  // drop if no out-record
  shared_ptr<Interest> interest4 = makeInterest("/Etab4KpY", 157);
  shared_ptr<pit::Entry> pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateOutRecord(face1, *interest4);

  lp::Nack nack4a = makeNack("/Etab4KpY", 157, lp::NackReason::CONGESTION);
  strategyP->afterReceiveNack_count = 0;
  strategyQ->afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face2, nack4a);
  BOOST_CHECK_EQUAL(strategyP->afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyQ->afterReceiveNack_count, 0);

  // drop if Nonce does not match out-record
  lp::Nack nack4b = makeNack("/Etab4KpY", 294, lp::NackReason::CONGESTION);
  strategyP->afterReceiveNack_count = 0;
  strategyQ->afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face1, nack4b);
  BOOST_CHECK_EQUAL(strategyP->afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyQ->afterReceiveNack_count, 0);

  // drop if inFace is multi-access
  pit4->insertOrUpdateOutRecord(face3, *interest4);
  strategyP->afterReceiveNack_count = 0;
  strategyQ->afterReceiveNack_count = 0;
  forwarder.onIncomingNack(*face3, nack4a);
  BOOST_CHECK_EQUAL(strategyP->afterReceiveNack_count, 0);
  BOOST_CHECK_EQUAL(strategyQ->afterReceiveNack_count, 0);
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
  pit1->insertOrUpdateInRecord(face1, *interest1);

  face2->sentNacks.clear();
  forwarder.onOutgoingNack(pit1, *face2, nackHeader);
  BOOST_CHECK_EQUAL(face2->sentNacks.size(), 0);

  // send Nack with correct Nonce
  shared_ptr<Interest> interest2a = makeInterest("/Vi8tRm9MG3", 152);
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2a).first;
  pit2->insertOrUpdateInRecord(face1, *interest2a);
  shared_ptr<Interest> interest2b = makeInterest("/Vi8tRm9MG3", 808);
  pit2->insertOrUpdateInRecord(face2, *interest2b);

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
  pit2->insertOrUpdateInRecord(face3, *interest2c);

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
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name("/zT4XwK0Hnx")).first;
  fibEntry->addNextHop(face4, 0);

  // receive Interest on face1
  face1->sentNacks.clear();
  shared_ptr<Interest> interest1a = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", 732);
  face1->receiveInterest(*interest1a);
  BOOST_CHECK(face1->sentNacks.empty());

  // receive Interest with duplicate Nonce on face1
  face1->sentNacks.clear();
  shared_ptr<Interest> interest1b = makeInterest("/zT4XwK0Hnx/28JBUvbEzc", 732);
  face1->receiveInterest(*interest1b);
  BOOST_REQUIRE_EQUAL(face1->sentNacks.size(), 1);
  BOOST_CHECK_EQUAL(face1->sentNacks.back().getInterest(), *interest1b);
  BOOST_CHECK_EQUAL(face1->sentNacks.back().getReason(), lp::NackReason::DUPLICATE);

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
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name("ndn:/A")).first;
  fibEntry->addNextHop(face2, 0);

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

BOOST_FIXTURE_TEST_CASE(PitLeak, UnitTestTimeFixture) // Bug 3484
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

BOOST_AUTO_TEST_CASE(LinkDelegation)
{
  Forwarder forwarder;
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  auto strategyP = make_shared<DummyStrategy>(ref(forwarder), "ndn:/strategyP");
  strategyP->wantAfterReceiveInterestCalls = true;
  strategyChoice.install(strategyP);
  strategyChoice.insert("ndn:/" , strategyP->getName());

  Fib& fib = forwarder.getFib();
  Pit& pit = forwarder.getPit();
  NetworkRegionTable& nrt = forwarder.getNetworkRegionTable();

  // returns prefix of FIB entry during last afterReceiveInterest trigger
  auto getLastFibPrefix = [strategyP] () -> Name {
    BOOST_REQUIRE(!strategyP->afterReceiveInterestCalls.empty());
    return std::get<2>(strategyP->afterReceiveInterestCalls.back())->getPrefix();
  };

  shared_ptr<Link> link = makeLink("/net/ndnsim", {{10, "/telia/terabits"}, {20, "/ucla/cs"}});

  // consumer region
  nrt.clear();
  nrt.insert("/arizona/cs/avenir");
  shared_ptr<fib::Entry> fibRoot = fib.insert("/").first;
  fibRoot->addNextHop(face2, 10);

  auto interest1 = makeInterest("/net/ndnsim/www/1.html");
  interest1->setLink(link->wireEncode());
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(face1, *interest1);

  forwarder.onContentStoreMiss(*face1, pit1, *interest1);
  BOOST_CHECK_EQUAL(getLastFibPrefix(), "/");
  BOOST_CHECK_EQUAL(interest1->hasSelectedDelegation(), false);

  fibRoot->removeNextHop(face2);

  // first default-free router, both delegations are available
  nrt.clear();
  nrt.insert("/arizona/cs/hobo");
  shared_ptr<fib::Entry> fibTelia = fib.insert("/telia").first;
  fibTelia->addNextHop(face2, 10);
  shared_ptr<fib::Entry> fibUcla = fib.insert("/ucla").first;
  fibUcla->addNextHop(face2, 10);

  auto interest2 = makeInterest("/net/ndnsim/www/2.html");
  interest2->setLink(link->wireEncode());
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateInRecord(face1, *interest2);

  forwarder.onContentStoreMiss(*face1, pit2, *interest2);
  BOOST_CHECK_EQUAL(getLastFibPrefix(), "/telia");
  BOOST_REQUIRE_EQUAL(interest2->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest2->getSelectedDelegation(), "/telia/terabits");

  fib.erase(*fibTelia);
  fib.erase(*fibUcla);

  // first default-free router, only second delegation is available
  nrt.clear();
  nrt.insert("/arizona/cs/hobo");
  fibUcla = fib.insert("/ucla").first;
  fibUcla->addNextHop(face2, 10);

  auto interest3 = makeInterest("/net/ndnsim/www/3.html");
  interest3->setLink(link->wireEncode());
  shared_ptr<pit::Entry> pit3 = pit.insert(*interest3).first;
  pit3->insertOrUpdateInRecord(face1, *interest3);

  forwarder.onContentStoreMiss(*face1, pit3, *interest3);
  BOOST_CHECK_EQUAL(getLastFibPrefix(), "/ucla");
  BOOST_REQUIRE_EQUAL(interest3->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest3->getSelectedDelegation(), "/ucla/cs");

  fib.erase(*fibUcla);

  // default-free router, chosen SelectedDelegation
  nrt.clear();
  nrt.insert("/ucsd/caida/click");
  fibTelia = fib.insert("/telia").first;
  fibTelia->addNextHop(face2, 10);
  fibUcla = fib.insert("/ucla").first;
  fibUcla->addNextHop(face2, 10);

  auto interest4 = makeInterest("/net/ndnsim/www/4.html");
  interest4->setLink(link->wireEncode());
  interest4->setSelectedDelegation("/ucla/cs");
  shared_ptr<pit::Entry> pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateInRecord(face1, *interest4);

  forwarder.onContentStoreMiss(*face1, pit4, *interest4);
  BOOST_CHECK_EQUAL(getLastFibPrefix(), "/ucla");
  BOOST_REQUIRE_EQUAL(interest4->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest4->getSelectedDelegation(), "/ucla/cs");

  fib.erase(*fibTelia);
  fib.erase(*fibUcla);

  // producer region
  nrt.clear();
  nrt.insert("/ucla/cs/spurs");
  fibRoot->addNextHop(face2, 10);
  fibUcla = fib.insert("/ucla").first;
  fibUcla->addNextHop(face2, 10);
  shared_ptr<fib::Entry> fibNdnsim = fib.insert("/net/ndnsim").first;
  fibNdnsim->addNextHop(face2, 10);

  auto interest5 = makeInterest("/net/ndnsim/www/5.html");
  interest5->setLink(link->wireEncode());
  interest5->setSelectedDelegation("/ucla/cs");
  shared_ptr<pit::Entry> pit5 = pit.insert(*interest5).first;
  pit5->insertOrUpdateInRecord(face1, *interest5);

  forwarder.onContentStoreMiss(*face1, pit5, *interest5);
  BOOST_CHECK_EQUAL(getLastFibPrefix(), "/net/ndnsim");
  BOOST_REQUIRE_EQUAL(interest5->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest5->getSelectedDelegation(), "/ucla/cs");

  fibRoot->removeNextHop(face2);
  fib.erase(*fibUcla);
  fib.erase(*fibNdnsim);
}


class MalformedPacketFixture : public UnitTestTimeFixture
{
protected:
  MalformedPacketFixture()
    : face1(make_shared<DummyFace>())
    , face2(make_shared<DummyFace>())
  {
    forwarder.addFace(face1);
    forwarder.addFace(face2);
  }

  void
  processInterest(shared_ptr<Interest> badInterest)
  {
    forwarder.startProcessInterest(*face1, *badInterest);
    this->continueProcessPacket();
  }

  // processData

  // processNack

private:
  void
  continueProcessPacket()
  {
    this->advanceClocks(time::milliseconds(10), time::seconds(6));
  }

protected:
  Forwarder forwarder;
  shared_ptr<DummyFace> face1; // face of incoming bad packet
  shared_ptr<DummyFace> face2; // another face for setting up states
};

BOOST_FIXTURE_TEST_SUITE(MalformedPacket, MalformedPacketFixture)

BOOST_AUTO_TEST_CASE(BadLink)
{
  shared_ptr<Interest> goodInterest = makeInterest("ndn:/");
  Block wire = goodInterest->wireEncode();
  wire.push_back(ndn::encoding::makeEmptyBlock(tlv::Data)); // bad Link
  wire.encode();

  auto badInterest = make_shared<Interest>();
  BOOST_REQUIRE_NO_THROW(badInterest->wireDecode(wire));
  BOOST_REQUIRE(badInterest->hasLink());
  BOOST_REQUIRE_THROW(badInterest->getLink(), tlv::Error);

  BOOST_CHECK_NO_THROW(this->processInterest(badInterest));
}

BOOST_AUTO_TEST_SUITE_END() // MalformedPacket

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
