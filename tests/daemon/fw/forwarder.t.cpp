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

#include "fw/forwarder.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "dummy-strategy.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwForwarder, BaseFixture)

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

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  face1->afterSend.connect(afterOp);
  face2->afterSend.connect(afterOp);
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name("ndn:/A")).first;
  fibEntry->addNextHop(face2, 0);

  BOOST_CHECK_EQUAL(forwarder.getCounters().getNInInterests (), 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().getNOutInterests(), 0);
  g_io.post([&] { face1->receiveInterest(*interestAB); });
  BOOST_CHECK_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
  BOOST_REQUIRE_EQUAL(face2->m_sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face2->m_sentInterests[0].getName(), nameAB);
  BOOST_CHECK_EQUAL(face2->m_sentInterests[0].getIncomingFaceId(), face1->getId());
  BOOST_CHECK_EQUAL(forwarder.getCounters().getNInInterests (), 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().getNOutInterests(), 1);

  BOOST_CHECK_EQUAL(forwarder.getCounters().getNInDatas (), 0);
  BOOST_CHECK_EQUAL(forwarder.getCounters().getNOutDatas(), 0);
  g_io.post([&] { face2->receiveData(*dataABC); });
  BOOST_CHECK_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
  BOOST_REQUIRE_EQUAL(face1->m_sentDatas.size(), 1);
  BOOST_CHECK_EQUAL(face1->m_sentDatas[0].getName(), nameABC);
  BOOST_CHECK_EQUAL(face1->m_sentDatas[0].getIncomingFaceId(), face2->getId());
  BOOST_CHECK_EQUAL(forwarder.getCounters().getNInDatas (), 1);
  BOOST_CHECK_EQUAL(forwarder.getCounters().getNOutDatas(), 1);
}

BOOST_AUTO_TEST_CASE(CsMatched)
{
  LimitedIo limitedIo;
  Forwarder forwarder;

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  shared_ptr<Interest> interestA = makeInterest("ndn:/A");
  interestA->setInterestLifetime(time::seconds(4));
  shared_ptr<Data> dataA = makeData("ndn:/A");
  dataA->setIncomingFaceId(face3->getId());

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name("ndn:/A")).first;
  fibEntry->addNextHop(face2, 0);

  Pit& pit = forwarder.getPit();
  BOOST_CHECK_EQUAL(pit.size(), 0);

  Cs& cs = forwarder.getCs();
  BOOST_REQUIRE(cs.insert(*dataA));

  face1->receiveInterest(*interestA);
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(5));
  // Interest matching ContentStore should not be forwarded
  BOOST_REQUIRE_EQUAL(face2->m_sentInterests.size(), 0);

  BOOST_REQUIRE_EQUAL(face1->m_sentDatas.size(), 1);
  // IncomingFaceId field should be reset to represent CS
  BOOST_CHECK_EQUAL(face1->m_sentDatas[0].getIncomingFaceId(), FACEID_CONTENT_STORE);

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
  onDataUnsolicited(Face& inFace, const Data& data)
  {
    ++m_onDataUnsolicited_count;
  }

protected:
  virtual void
  dispatchToStrategy(shared_ptr<pit::Entry> pitEntry, function<void(fw::Strategy*)> f)
  {
    ++m_dispatchToStrategy_count;
  }

public:
  int m_dispatchToStrategy_count;
  int m_onDataUnsolicited_count;
};

BOOST_AUTO_TEST_CASE(ScopeLocalhostIncoming)
{
  ScopeLocalhostIncomingTestForwarder forwarder;
  shared_ptr<Face> face1 = make_shared<DummyLocalFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  // local face, /localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  shared_ptr<Interest> i1 = makeInterest("/localhost/A1");
  forwarder.onIncomingInterest(*face1, *i1);
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // non-local face, /localhost: violate
  forwarder.m_dispatchToStrategy_count = 0;
  shared_ptr<Interest> i2 = makeInterest("/localhost/A2");
  forwarder.onIncomingInterest(*face2, *i2);
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 0);

  // local face, non-/localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  shared_ptr<Interest> i3 = makeInterest("/A3");
  forwarder.onIncomingInterest(*face1, *i3);
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  shared_ptr<Interest> i4 = makeInterest("/A4");
  forwarder.onIncomingInterest(*face2, *i4);
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // local face, /localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  shared_ptr<Data> d1 = makeData("/localhost/B1");
  forwarder.onIncomingData(*face1, *d1);
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);

  // non-local face, /localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  shared_ptr<Data> d2 = makeData("/localhost/B2");
  forwarder.onIncomingData(*face2, *d2);
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 0);

  // local face, non-/localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  shared_ptr<Data> d3 = makeData("/B3");
  forwarder.onIncomingData(*face1, *d3);
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  shared_ptr<Data> d4 = makeData("/B4");
  forwarder.onIncomingData(*face2, *d4);
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);
}

BOOST_AUTO_TEST_CASE(ScopeLocalhostOutgoing)
{
  Forwarder forwarder;
  shared_ptr<DummyLocalFace> face1 = make_shared<DummyLocalFace>();
  shared_ptr<DummyFace>      face2 = make_shared<DummyFace>();
  shared_ptr<Face>           face3 = make_shared<DummyLocalFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  Pit& pit = forwarder.getPit();

  // local face, /localhost: OK
  shared_ptr<Interest> interestA1 = makeInterest("/localhost/A1");
  shared_ptr<pit::Entry> pitA1 = pit.insert(*interestA1).first;
  pitA1->insertOrUpdateInRecord(face3, *interestA1);
  face1->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pitA1, *face1);
  BOOST_CHECK_EQUAL(face1->m_sentInterests.size(), 1);

  // non-local face, /localhost: violate
  shared_ptr<Interest> interestA2 = makeInterest("/localhost/A2");
  shared_ptr<pit::Entry> pitA2 = pit.insert(*interestA2).first;
  pitA2->insertOrUpdateInRecord(face3, *interestA2);
  face2->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pitA2, *face2);
  BOOST_CHECK_EQUAL(face2->m_sentInterests.size(), 0);

  // local face, non-/localhost: OK
  shared_ptr<Interest> interestA3 = makeInterest("/A3");
  shared_ptr<pit::Entry> pitA3 = pit.insert(*interestA3).first;
  pitA3->insertOrUpdateInRecord(face3, *interestA3);
  face1->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pitA3, *face1);
  BOOST_CHECK_EQUAL(face1->m_sentInterests.size(), 1);

  // non-local face, non-/localhost: OK
  shared_ptr<Interest> interestA4 = makeInterest("/A4");
  shared_ptr<pit::Entry> pitA4 = pit.insert(*interestA4).first;
  pitA4->insertOrUpdateInRecord(face3, *interestA4);
  face2->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pitA4, *face2);
  BOOST_CHECK_EQUAL(face2->m_sentInterests.size(), 1);

  // local face, /localhost: OK
  face1->m_sentDatas.clear();
  forwarder.onOutgoingData(Data("/localhost/B1"), *face1);
  BOOST_CHECK_EQUAL(face1->m_sentDatas.size(), 1);

  // non-local face, /localhost: OK
  face2->m_sentDatas.clear();
  forwarder.onOutgoingData(Data("/localhost/B2"), *face2);
  BOOST_CHECK_EQUAL(face2->m_sentDatas.size(), 0);

  // local face, non-/localhost: OK
  face1->m_sentDatas.clear();
  forwarder.onOutgoingData(Data("/B3"), *face1);
  BOOST_CHECK_EQUAL(face1->m_sentDatas.size(), 1);

  // non-local face, non-/localhost: OK
  face2->m_sentDatas.clear();
  forwarder.onOutgoingData(Data("/B4"), *face2);
  BOOST_CHECK_EQUAL(face2->m_sentDatas.size(), 1);
}

BOOST_AUTO_TEST_CASE(ScopeLocalhopOutgoing)
{
  Forwarder forwarder;
  shared_ptr<DummyLocalFace> face1 = make_shared<DummyLocalFace>();
  shared_ptr<DummyFace>      face2 = make_shared<DummyFace>();
  shared_ptr<DummyLocalFace> face3 = make_shared<DummyLocalFace>();
  shared_ptr<DummyFace>      face4 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  forwarder.addFace(face4);
  Pit& pit = forwarder.getPit();

  // from local face, to local face: OK
  shared_ptr<Interest> interest1 = makeInterest("/localhop/1");
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(face1, *interest1);
  face3->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pit1, *face3);
  BOOST_CHECK_EQUAL(face3->m_sentInterests.size(), 1);

  // from non-local face, to local face: OK
  shared_ptr<Interest> interest2 = makeInterest("/localhop/2");
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateInRecord(face2, *interest2);
  face3->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pit2, *face3);
  BOOST_CHECK_EQUAL(face3->m_sentInterests.size(), 1);

  // from local face, to non-local face: OK
  shared_ptr<Interest> interest3 = makeInterest("/localhop/3");
  shared_ptr<pit::Entry> pit3 = pit.insert(*interest3).first;
  pit3->insertOrUpdateInRecord(face1, *interest3);
  face4->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pit3, *face4);
  BOOST_CHECK_EQUAL(face4->m_sentInterests.size(), 1);

  // from non-local face, to non-local face: violate
  shared_ptr<Interest> interest4 = makeInterest("/localhop/4");
  shared_ptr<pit::Entry> pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateInRecord(face2, *interest4);
  face4->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pit4, *face4);
  BOOST_CHECK_EQUAL(face4->m_sentInterests.size(), 0);

  // from local face and non-local face, to local face: OK
  shared_ptr<Interest> interest5 = makeInterest("/localhop/5");
  shared_ptr<pit::Entry> pit5 = pit.insert(*interest5).first;
  pit5->insertOrUpdateInRecord(face1, *interest5);
  pit5->insertOrUpdateInRecord(face2, *interest5);
  face3->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pit5, *face3);
  BOOST_CHECK_EQUAL(face3->m_sentInterests.size(), 1);

  // from local face and non-local face, to non-local face: OK
  shared_ptr<Interest> interest6 = makeInterest("/localhop/6");
  shared_ptr<pit::Entry> pit6 = pit.insert(*interest6).first;
  pit6->insertOrUpdateInRecord(face1, *interest6);
  pit6->insertOrUpdateInRecord(face2, *interest6);
  face4->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pit6, *face4);
  BOOST_CHECK_EQUAL(face4->m_sentInterests.size(), 1);
}

BOOST_AUTO_TEST_CASE(StrategyDispatch)
{
  LimitedIo limitedIo;
  Forwarder forwarder;
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
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
  strategyP->m_afterReceiveInterest_count = 0;
  strategyP->m_interestOutFace = face2;
  forwarder.onInterest(*face1, *interest1);
  BOOST_CHECK_EQUAL(strategyP->m_afterReceiveInterest_count, 1);

  shared_ptr<Interest> interest2 = makeInterest("ndn:/B/2");
  strategyQ->m_afterReceiveInterest_count = 0;
  strategyQ->m_interestOutFace = face2;
  forwarder.onInterest(*face1, *interest2);
  BOOST_CHECK_EQUAL(strategyQ->m_afterReceiveInterest_count, 1);

  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(5));

  shared_ptr<Data> data1 = makeData("ndn:/A/1/a");
  strategyP->m_beforeSatisfyInterest_count = 0;
  forwarder.onData(*face2, *data1);
  BOOST_CHECK_EQUAL(strategyP->m_beforeSatisfyInterest_count, 1);

  shared_ptr<Data> data2 = makeData("ndn:/B/2/b");
  strategyQ->m_beforeSatisfyInterest_count = 0;
  forwarder.onData(*face2, *data2);
  BOOST_CHECK_EQUAL(strategyQ->m_beforeSatisfyInterest_count, 1);

  shared_ptr<Interest> interest3 = makeInterest("ndn:/A/3");
  interest3->setInterestLifetime(time::milliseconds(30));
  forwarder.onInterest(*face1, *interest3);
  shared_ptr<Interest> interest4 = makeInterest("ndn:/B/4");
  interest4->setInterestLifetime(time::milliseconds(5000));
  forwarder.onInterest(*face1, *interest4);

  strategyP->m_beforeExpirePendingInterest_count = 0;
  strategyQ->m_beforeExpirePendingInterest_count = 0;
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(100));
  BOOST_CHECK_EQUAL(strategyP->m_beforeExpirePendingInterest_count, 1);
  BOOST_CHECK_EQUAL(strategyQ->m_beforeExpirePendingInterest_count, 0);
}

BOOST_AUTO_TEST_CASE(IncomingData)
{
  LimitedIo limitedIo;
  Forwarder forwarder;
  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face4 = make_shared<DummyFace>();
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

  BOOST_CHECK_EQUAL(face1->m_sentDatas.size(), 1);
  BOOST_CHECK_EQUAL(face2->m_sentDatas.size(), 1);
  BOOST_CHECK_EQUAL(face3->m_sentDatas.size(), 0);
  BOOST_CHECK_EQUAL(face4->m_sentDatas.size(), 1);
}

BOOST_FIXTURE_TEST_CASE(InterestLoopWithShortLifetime, UnitTestTimeFixture) // Bug 1953
{
  Forwarder forwarder;
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  // cause an Interest sent out of face2 to loop back into face1 after a delay
  face2->onSendInterest.connect([&face1] (const Interest& interest) {
    scheduler::schedule(time::milliseconds(170), [&] { face1->receiveInterest(interest); });
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

  BOOST_CHECK_EQUAL(face2->m_sentInterests.size(), 1);

  // It's unnecessary to check that Interest with duplicate Nonce can be forwarded again
  // after it's gone from Dead Nonce List, because the entry lifetime of Dead Nonce List
  // is an implementation decision. NDN protocol requires Name+Nonce to be unique,
  // without specifying when Name+Nonce could repeat. Forwarder is permitted to suppress
  // an Interest if its Name+Nonce has appeared any point in the past.
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
