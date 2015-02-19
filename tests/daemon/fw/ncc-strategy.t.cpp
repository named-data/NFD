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

#include "fw/ncc-strategy.hpp"
#include "strategy-tester.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "tests/limited-io.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_FIXTURE_TEST_SUITE(FwNccStrategy, UnitTestTimeFixture)

// NccStrategy is fairly complex.
// The most important property is:
// it remembers which upstream is the fastest to return Data,
// and favors this upstream in subsequent Interests.
BOOST_AUTO_TEST_CASE(FavorRespondingUpstream)
{
  LimitedIo limitedIo(this);
  Forwarder forwarder;
  typedef StrategyTester<fw::NccStrategy> NccStrategyTester;
  shared_ptr<NccStrategyTester> strategy = make_shared<NccStrategyTester>(ref(forwarder));
  strategy->onAction.connect(bind(&LimitedIo::afterOp, &limitedIo));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face1, 10);
  fibEntry->addNextHop(face2, 20);

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  strategyChoice.install(strategy);
  strategyChoice.insert(Name(), strategy->getName());

  Pit& pit = forwarder.getPit();

  // first Interest: strategy knows nothing and follows routing
  shared_ptr<Interest> interest1p = makeInterest("ndn:/0Jm1ajrW/%00");
  Interest& interest1 = *interest1p;
  interest1.setInterestLifetime(time::milliseconds(8000));
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(interest1).first;

  pitEntry1->insertOrUpdateInRecord(face3, interest1);
  strategy->afterReceiveInterest(*face3, interest1, fibEntry, pitEntry1);

  // forwards to face1 because routing says it's best
  // (no io run here: afterReceiveInterest has already sent the Interest)
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[0].get<1>(), face1);

  // forwards to face2 because face1 doesn't respond
  limitedIo.run(1, time::milliseconds(500), time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[1].get<1>(), face2);

  // face2 responds
  shared_ptr<Data> data1p = makeData("ndn:/0Jm1ajrW/%00");
  Data& data1 = *data1p;
  strategy->beforeSatisfyInterest(pitEntry1, *face2, data1);
  this->advanceClocks(time::milliseconds(10), time::milliseconds(500));

  // second Interest: strategy knows face2 is best
  shared_ptr<Interest> interest2p = makeInterest("ndn:/0Jm1ajrW/%00%01");
  Interest& interest2 = *interest2p;
  interest2.setInterestLifetime(time::milliseconds(8000));
  shared_ptr<pit::Entry> pitEntry2 = pit.insert(interest2).first;

  pitEntry2->insertOrUpdateInRecord(face3, interest2);
  strategy->afterReceiveInterest(*face3, interest2, fibEntry, pitEntry2);

  // forwards to face2 because it responds previously
  this->advanceClocks(time::milliseconds(1));
  BOOST_REQUIRE_GE(strategy->m_sendInterestHistory.size(), 3);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[2].get<1>(), face2);
}

BOOST_AUTO_TEST_CASE(Bug1853)
{
  Forwarder forwarder;
  typedef StrategyTester<fw::NccStrategy> NccStrategyTester;
  shared_ptr<NccStrategyTester> strategy = make_shared<NccStrategyTester>(ref(forwarder));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face1, 10);

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  strategyChoice.install(strategy);
  strategyChoice.insert(Name(), strategy->getName());

  Pit& pit = forwarder.getPit();

  // first Interest: strategy follows routing and forwards to face1
  shared_ptr<Interest> interest1 = makeInterest("ndn:/nztwIvHX/%00");
  interest1->setInterestLifetime(time::milliseconds(8000));
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;

  pitEntry1->insertOrUpdateInRecord(face3, *interest1);
  strategy->afterReceiveInterest(*face3, *interest1, fibEntry, pitEntry1);

  this->advanceClocks(time::milliseconds(1));
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[0].get<1>(), face1);

  // face1 responds
  shared_ptr<Data> data1 = makeData("ndn:/nztwIvHX/%00");
  strategy->beforeSatisfyInterest(pitEntry1, *face1, *data1);
  this->advanceClocks(time::milliseconds(10), time::milliseconds(500));

  // second Interest: bestFace is face1, nUpstreams becomes 0,
  // therefore pitEntryInfo->maxInterval cannot be calculated from deferRange and nUpstreams
  shared_ptr<Interest> interest2 = makeInterest("ndn:/nztwIvHX/%01");
  interest2->setInterestLifetime(time::milliseconds(8000));
  shared_ptr<pit::Entry> pitEntry2 = pit.insert(*interest2).first;

  pitEntry2->insertOrUpdateInRecord(face3, *interest2);
  strategy->afterReceiveInterest(*face3, *interest2, fibEntry, pitEntry2);

  // FIB entry is changed before doPropagate executes
  fibEntry->addNextHop(face2, 20);
  this->advanceClocks(time::milliseconds(10), time::milliseconds(1000));// should not crash
}

BOOST_AUTO_TEST_CASE(Bug1961)
{
  LimitedIo limitedIo(this);
  Forwarder forwarder;
  typedef StrategyTester<fw::NccStrategy> NccStrategyTester;
  shared_ptr<NccStrategyTester> strategy = make_shared<NccStrategyTester>(ref(forwarder));
  strategy->onAction.connect(bind(&LimitedIo::afterOp, &limitedIo));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face3 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face1, 10);
  fibEntry->addNextHop(face2, 20);

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  strategyChoice.install(strategy);
  strategyChoice.insert(Name(), strategy->getName());

  Pit& pit = forwarder.getPit();

  // first Interest: strategy forwards to face1 and face2
  shared_ptr<Interest> interest1 = makeInterest("ndn:/seRMz5a6/%00");
  interest1->setInterestLifetime(time::milliseconds(2000));
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;

  pitEntry1->insertOrUpdateInRecord(face3, *interest1);
  strategy->afterReceiveInterest(*face3, *interest1, fibEntry, pitEntry1);
  limitedIo.run(2 - strategy->m_sendInterestHistory.size(),
                time::milliseconds(2000), time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[0].get<1>(), face1);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[1].get<1>(), face2);

  // face1 responds
  shared_ptr<Data> data1 = makeData("ndn:/seRMz5a6/%00");
  strategy->beforeSatisfyInterest(pitEntry1, *face1, *data1);
  pitEntry1->deleteInRecords();
  this->advanceClocks(time::milliseconds(10));
  // face2 also responds
  strategy->beforeSatisfyInterest(pitEntry1, *face2, *data1);
  this->advanceClocks(time::milliseconds(10));

  // second Interest: bestFace should be face 1
  shared_ptr<Interest> interest2 = makeInterest("ndn:/seRMz5a6/%01");
  interest2->setInterestLifetime(time::milliseconds(2000));
  shared_ptr<pit::Entry> pitEntry2 = pit.insert(*interest2).first;

  pitEntry2->insertOrUpdateInRecord(face3, *interest2);
  strategy->afterReceiveInterest(*face3, *interest2, fibEntry, pitEntry2);
  limitedIo.run(3 - strategy->m_sendInterestHistory.size(),
                time::milliseconds(2000), time::milliseconds(10));

  BOOST_REQUIRE_GE(strategy->m_sendInterestHistory.size(), 3);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[2].get<1>(), face1);
}

BOOST_AUTO_TEST_CASE(Bug1971)
{
  LimitedIo limitedIo(this);
  Forwarder forwarder;
  typedef StrategyTester<fw::NccStrategy> NccStrategyTester;
  shared_ptr<NccStrategyTester> strategy = make_shared<NccStrategyTester>(ref(forwarder));
  strategy->onAction.connect(bind(&LimitedIo::afterOp, &limitedIo));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face2, 10);

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  strategyChoice.install(strategy);
  strategyChoice.insert(Name(), strategy->getName());

  Pit& pit = forwarder.getPit();

  // first Interest: strategy forwards to face2
  shared_ptr<Interest> interest1 = makeInterest("ndn:/M4mBXCsd");
  interest1->setInterestLifetime(time::milliseconds(2000));
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;

  pitEntry1->insertOrUpdateInRecord(face1, *interest1);
  strategy->afterReceiveInterest(*face1, *interest1, fibEntry, pitEntry1);
  limitedIo.run(1 - strategy->m_sendInterestHistory.size(),
                time::milliseconds(2000), time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[0].get<1>(), face2);

  // face2 responds
  shared_ptr<Data> data1 = makeData("ndn:/M4mBXCsd");
  data1->setFreshnessPeriod(time::milliseconds(5));
  strategy->beforeSatisfyInterest(pitEntry1, *face2, *data1);
  pitEntry1->deleteOutRecord(*face2);
  pitEntry1->deleteInRecords();
  this->advanceClocks(time::milliseconds(10));

  // similar Interest: strategy should still forward it
  pitEntry1->insertOrUpdateInRecord(face1, *interest1);
  strategy->afterReceiveInterest(*face1, *interest1, fibEntry, pitEntry1);
  limitedIo.run(2 - strategy->m_sendInterestHistory.size(),
                time::milliseconds(2000), time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 2);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[1].get<1>(), face2);
}

BOOST_AUTO_TEST_CASE(Bug1998)
{
  Forwarder forwarder;
  typedef StrategyTester<fw::NccStrategy> NccStrategyTester;
  shared_ptr<NccStrategyTester> strategy = make_shared<NccStrategyTester>(ref(forwarder));

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(Name()).first;
  fibEntry->addNextHop(face1, 10); // face1 is top-ranked nexthop
  fibEntry->addNextHop(face2, 20);

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  strategyChoice.install(strategy);
  strategyChoice.insert(Name(), strategy->getName());

  Pit& pit = forwarder.getPit();

  // Interest comes from face1, which is sole downstream
  shared_ptr<Interest> interest1 = makeInterest("ndn:/tFy5HzUzD4");
  shared_ptr<pit::Entry> pitEntry1 = pit.insert(*interest1).first;
  pitEntry1->insertOrUpdateInRecord(face1, *interest1);

  strategy->afterReceiveInterest(*face1, *interest1, fibEntry, pitEntry1);

  // Interest shall go to face2, not loop back to face1
  BOOST_REQUIRE_EQUAL(strategy->m_sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy->m_sendInterestHistory[0].get<1>(), face2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace fw
} // namespace nfd
