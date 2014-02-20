/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fw/broadcast-strategy.hpp"
#include "strategy-tester.hpp"
#include "../face/dummy-face.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(FwBroadcastStrategy)

BOOST_AUTO_TEST_CASE(ForwardTwo)
{
  boost::asio::io_service io;
  Forwarder forwarder(io);
  typedef StrategyTester<fw::BroadcastStrategy> BroadcastStrategyTester;
  BroadcastStrategyTester strategy(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  std::pair<shared_ptr<fib::Entry>, bool> fibInsertResult = fib.insert(Name());
  shared_ptr<fib::Entry> fibEntry = fibInsertResult.first;
  fibEntry->addNextHop(face1, 0);
  fibEntry->addNextHop(face2, 0);
  fibEntry->addNextHop(face3, 0);

  Interest interest(Name("ndn:/H0D6i5fc"));
  Pit& pit = forwarder.getPit();
  std::pair<shared_ptr<pit::Entry>, bool> pitInsertResult = pit.insert(interest);
  shared_ptr<pit::Entry> pitEntry = pitInsertResult.first;

  strategy.afterReceiveInterest(*face3, interest, fibEntry, pitEntry);
  BOOST_CHECK_EQUAL(strategy.m_rebuffPendingInterestHistory.size(), 0);
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

BOOST_AUTO_TEST_CASE(Rebuff)
{
  boost::asio::io_service io;
  Forwarder forwarder(io);
  typedef StrategyTester<fw::BroadcastStrategy> BroadcastStrategyTester;
  BroadcastStrategyTester strategy(forwarder);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  forwarder.addFace(face1);

  Fib& fib = forwarder.getFib();
  std::pair<shared_ptr<fib::Entry>, bool> fibInsertResult = fib.insert(Name());
  shared_ptr<fib::Entry> fibEntry = fibInsertResult.first;
  fibEntry->addNextHop(face1, 0);

  Interest interest(Name("ndn:/H0D6i5fc"));
  Pit& pit = forwarder.getPit();
  std::pair<shared_ptr<pit::Entry>, bool> pitInsertResult = pit.insert(interest);
  shared_ptr<pit::Entry> pitEntry = pitInsertResult.first;

  strategy.afterReceiveInterest(*face1, interest, fibEntry, pitEntry);
  BOOST_CHECK_EQUAL(strategy.m_rebuffPendingInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.m_sendInterestHistory.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
