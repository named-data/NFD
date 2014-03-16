/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fw/broadcast-strategy.hpp"
#include "strategy-tester.hpp"
#include "tests/face/dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwBroadcastStrategy, BaseFixture)

BOOST_AUTO_TEST_CASE(Forward2)
{
  Forwarder forwarder;
  typedef StrategyTester<fw::BroadcastStrategy> BroadcastStrategyTester;
  BroadcastStrategyTester strategy(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face1, 0);
  fibEntry->addNextHop(face2, 0);
  fibEntry->addNextHop(face3, 0);

  shared_ptr<Interest> interest = makeInterest("ndn:/H0D6i5fc");
  Pit& pit = forwarder.getPit();
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(face3, *interest);

  strategy.afterReceiveInterest(*face3, *interest, fibEntry, pitEntry);
  BOOST_CHECK_EQUAL(strategy.m_rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.size(), 2);
  bool hasFace1 = false;
  bool hasFace2 = false;
  for (std::vector<BroadcastStrategyTester::SendInterestArgs>::iterator it =
       strategy.m_sendInterestHistory.begin();
       it != strategy.m_sendInterestHistory.end(); ++it) {
    if (it->get<1>() == face1) {
      hasFace1 = true;
    }
    if (it->get<1>() == face2) {
      hasFace2 = true;
    }
  }
  BOOST_CHECK(hasFace1 && hasFace2);
}

BOOST_AUTO_TEST_CASE(RejectScope)
{
  Forwarder forwarder;
  typedef StrategyTester<fw::BroadcastStrategy> BroadcastStrategyTester;
  BroadcastStrategyTester strategy(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert("ndn:/localhop/uS09bub6tm").first;
  fibEntry->addNextHop(face2, 0);

  shared_ptr<Interest> interest = makeInterest("ndn:/localhop/uS09bub6tm/eG3MMoP6z");
  Pit& pit = forwarder.getPit();
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(face1, *interest);

  strategy.afterReceiveInterest(*face1, *interest, fibEntry, pitEntry);
  BOOST_CHECK_EQUAL(strategy.m_rejectPendingInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.size(), 0);
}

BOOST_AUTO_TEST_CASE(RejectLoopback)
{
  Forwarder forwarder;
  typedef StrategyTester<fw::BroadcastStrategy> BroadcastStrategyTester;
  BroadcastStrategyTester strategy(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  forwarder.addFace(face1);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face1, 0);

  shared_ptr<Interest> interest = makeInterest("ndn:/H0D6i5fc");
  Pit& pit = forwarder.getPit();
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(face1, *interest);

  strategy.afterReceiveInterest(*face1, *interest, fibEntry, pitEntry);
  BOOST_CHECK_EQUAL(strategy.m_rejectPendingInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
