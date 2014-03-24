/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fw/client-control-strategy.hpp"
#include "strategy-tester.hpp"
#include "tests/face/dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwClientControlStrategy, BaseFixture)

BOOST_AUTO_TEST_CASE(Forward3)
{
  Forwarder forwarder;
  typedef StrategyTester<fw::ClientControlStrategy> ClientControlStrategyTester;
  ClientControlStrategyTester strategy(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  shared_ptr<DummyLocalFace> face4 = make_shared<DummyLocalFace>();
  face4->setLocalControlHeaderFeature(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID);
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  forwarder.addFace(face4);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face2, 0);

  Pit& pit = forwarder.getPit();

  // Interest with valid NextHopFaceId
  shared_ptr<Interest> interest1 = makeInterest("ndn:/0z8r6yDDe");
  interest1->setNextHopFaceId(face1->getId());
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;
  pitEntry1->insertOrUpdateInRecord(face4, *interest1);

  strategy.m_sendInterestHistory.clear();
  strategy.afterReceiveInterest(*face4, *interest1, fibEntry, pitEntry1);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory[0].get<1>(), face1);

  // Interest without NextHopFaceId
  shared_ptr<Interest> interest2 = makeInterest("ndn:/y6JQADGVz");
  shared_ptr<pit::Entry> pitEntry2 = pit.insert(*interest2).first;
  pitEntry2->insertOrUpdateInRecord(face4, *interest2);

  strategy.m_sendInterestHistory.clear();
  strategy.afterReceiveInterest(*face4, *interest2, fibEntry, pitEntry2);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory[0].get<1>(), face2);

  // Interest with invalid NextHopFaceId
  shared_ptr<Interest> interest3 = makeInterest("ndn:/0z8r6yDDe");
  interest3->setNextHopFaceId(face3->getId());
  shared_ptr<pit::Entry> pitEntry3 = pit.insert(*interest3).first;
  pitEntry3->insertOrUpdateInRecord(face4, *interest3);

  face3->close(); // face3 is closed and its FaceId becomes invalid
  strategy.m_sendInterestHistory.clear();
  strategy.m_rejectPendingInterestHistory.clear();
  strategy.afterReceiveInterest(*face4, *interest3, fibEntry, pitEntry3);
  BOOST_REQUIRE_EQUAL(strategy.m_sendInterestHistory.size(), 0);
  BOOST_REQUIRE_EQUAL(strategy.m_rejectPendingInterestHistory.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
