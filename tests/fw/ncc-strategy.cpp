/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fw/ncc-strategy.hpp"
#include "strategy-tester.hpp"
#include "tests/face/dummy-face.hpp"
#include "tests/core/limited-io.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwNccStrategy, BaseFixture)

// NccStrategy is fairly complex.
// The most important property is:
// it remembers which upstream is the fastest to return Data,
// and favors this upstream in subsequent Interests.
BOOST_AUTO_TEST_CASE(FavorRespondingUpstream)
{
  LimitedIo limitedIo;
  Forwarder forwarder;
  typedef StrategyTester<fw::NccStrategy> NccStrategyTester;
  shared_ptr<NccStrategyTester> strategy = make_shared<NccStrategyTester>(boost::ref(forwarder));
  strategy->onAction += bind(&LimitedIo::afterOp, &limitedIo);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->setStrategy(strategy);
  fibEntry->addNextHop(face1, 10);
  fibEntry->addNextHop(face2, 20);

  Pit& pit = forwarder.getPit();

  // first Interest: strategy knows nothing and follows routing
  shared_ptr<Interest> interest1p = make_shared<Interest>("ndn:/0Jm1ajrW/%00");
  Interest& interest1 = *interest1p;
  interest1.setInterestLifetime(8000);
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(interest1).first;

  pitEntry1->insertOrUpdateInRecord(face3, interest1);
  strategy->afterReceiveInterest(*face3, interest1, fibEntry, pitEntry1);

  // forwards to face1 because routing says it's best
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(1));
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[0].get<1>(), face1);

  // forwards to face2 because face1 doesn't respond
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(500));
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[1].get<1>(), face2);

  // face2 responds
  shared_ptr<Data> data1p = make_shared<Data>("ndn:/0Jm1ajrW/%00");
  Data& data1 = *data1p;
  strategy->beforeSatisfyPendingInterest(pitEntry1, *face2, data1);
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(500));

  // second Interest: strategy knows face2 is best
  shared_ptr<Interest> interest2p = make_shared<Interest>("ndn:/0Jm1ajrW/%00%01");
  Interest& interest2 = *interest2p;
  interest2.setInterestLifetime(8000);
  shared_ptr<pit::Entry> pitEntry2 = pit.insert(interest2).first;

  pitEntry2->insertOrUpdateInRecord(face3, interest2);
  strategy->afterReceiveInterest(*face3, interest2, fibEntry, pitEntry2);

  // forwards to face2 because it responds previously
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::milliseconds(1));
  BOOST_REQUIRE_GE(strategy->m_sendInterestHistory.size(), 3);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[2].get<1>(), face2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
