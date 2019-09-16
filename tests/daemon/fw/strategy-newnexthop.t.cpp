/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

namespace nfd {
namespace tests {

class StrategyNewNextHopFixture : public GlobalIoTimeFixture
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
BOOST_FIXTURE_TEST_SUITE(TestStrategyNewNextHop, StrategyNewNextHopFixture)

BOOST_AUTO_TEST_CASE(AfterNewNextHop1)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace();

  DummyStrategy& strategy = choose<DummyStrategy>(forwarder, "/A", DummyStrategy::getStrategyName());
  StrategyChoice& sc = forwarder.getStrategyChoice();
  sc.insert(Name("/A/B"), strategy.getStrategyName());
  sc.insert(Name("/A/B/C"), strategy.getStrategyName());

  strategy.enableNewNextHopTrigger(true);

  Fib& fib = forwarder.getFib();
  Pit& pit = forwarder.getPit();

  // fib: "/", "/A/B"
  fib::Entry* entry = fib.insert("/").first;
  fib.addOrUpdateNextHop(*entry, *face1, 0);
  entry = fib.insert("/A/B").first;
  fib.addOrUpdateNextHop(*entry, *face1, 0);

  // pit: "/A", "/A/B/C"
  auto interest1 = makeInterest("/A");
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(*face3, *interest1);
  auto interest2 = makeInterest("/A/B/C");
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateInRecord(*face3, *interest2);
  // new nexthop for "/"
  entry = fib.insert("/").first;
  fib.addOrUpdateNextHop(*entry, *face2, 0);

  // /A --> triggered, /A/B --> not triggered, /A/B/C --> not triggered
  BOOST_REQUIRE_EQUAL(strategy.afterNewNextHopCalls.size(), 1);
  BOOST_CHECK_EQUAL(strategy.afterNewNextHopCalls[0], Name("/A"));
}

BOOST_AUTO_TEST_CASE(AfterNewNextHop2)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace();

  DummyStrategy& strategy = choose<DummyStrategy>(forwarder, "/A", DummyStrategy::getStrategyName());
  StrategyChoice& sc = forwarder.getStrategyChoice();
  sc.insert(Name("/A/B"), strategy.getStrategyName());
  sc.insert(Name("/A/B/C"), strategy.getStrategyName());

  strategy.enableNewNextHopTrigger(true);

  Fib& fib = forwarder.getFib();
  Pit& pit = forwarder.getPit();

  // fib: "/", "/A/B"
  fib::Entry* entry = fib.insert("/").first;
  fib.addOrUpdateNextHop(*entry, *face1, 0);
  entry = fib.insert("/A/B").first;
  fib.addOrUpdateNextHop(*entry, *face1, 0);

  // pit: "/A", "/A/B/C"
  auto interest1 = makeInterest("/A");
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(*face3, *interest1);
  auto interest2 = makeInterest("/A/B");
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateInRecord(*face3, *interest2);
  // new nexthop for "/"
  entry = fib.insert("/").first;
  fib.addOrUpdateNextHop(*entry, *face2, 0);

  // /A --> triggered, /A/B --> not triggered, /A/B/C --> not triggered
  BOOST_REQUIRE_EQUAL(strategy.afterNewNextHopCalls.size(), 1);
  BOOST_CHECK_EQUAL(strategy.afterNewNextHopCalls[0], Name("/A"));
}

BOOST_AUTO_TEST_CASE(DisableTrigger)
{
  auto face1 = addFace();
  auto face2 = addFace();
  auto face3 = addFace();

  DummyStrategy& strategy = choose<DummyStrategy>(forwarder, "/A", DummyStrategy::getStrategyName());
  strategy.enableNewNextHopTrigger(false);

  Fib& fib = forwarder.getFib();
  Pit& pit = forwarder.getPit();

  fib::Entry* entry = fib.insert("/").first;
  fib.addOrUpdateNextHop(*entry, *face1, 0);

  auto interest1 = makeInterest("/A");
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(*face3, *interest1);
  // new nexthop for "/A"
  entry = fib.insert("/A").first;
  fib.addOrUpdateNextHop(*entry, *face2, 0);

  BOOST_CHECK_EQUAL(strategy.afterNewNextHopCalls.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyNewNextHop
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace nfd
