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

#include "fw/multicast-strategy.hpp"
#include "strategy-tester.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_FIXTURE_TEST_SUITE(FwMulticastStrategy, BaseFixture)

BOOST_AUTO_TEST_CASE(Forward2)
{
  Forwarder forwarder;
  typedef StrategyTester<fw::MulticastStrategy> MulticastStrategyTester;
  MulticastStrategyTester strategy(forwarder);

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
  for (std::vector<MulticastStrategyTester::SendInterestArgs>::iterator it =
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
  typedef StrategyTester<fw::MulticastStrategy> MulticastStrategyTester;
  MulticastStrategyTester strategy(forwarder);

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
  typedef StrategyTester<fw::MulticastStrategy> MulticastStrategyTester;
  MulticastStrategyTester strategy(forwarder);

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
} // namespace fw
} // namespace nfd
