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

#include "fw/client-control-strategy.hpp"
#include "strategy-tester.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

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
} // namespace fw
} // namespace nfd
