/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

/** \file
 *  This test suite checks that a strategy returns Nack-NoRoute
 *  when there is no usable FIB nexthop.
 */

// Strategies returning Nack-NoRoute when there is no usable FIB nexthop,
// sorted alphabetically.
#include "fw/asf-strategy.hpp"
#include "fw/best-route-strategy2.hpp"
#include "fw/multicast-strategy.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"
#include "choose-strategy.hpp"
#include "strategy-tester.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include <boost/mpl/copy_if.hpp>
#include <boost/mpl/vector.hpp>

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

template<typename S>
class StrategyNoRouteFixture : public UnitTestTimeFixture
{
public:
  StrategyNoRouteFixture()
    : limitedIo(this)
    , strategy(choose<StrategyTester<S>>(forwarder))
    , fib(forwarder.getFib())
    , pit(forwarder.getPit())
    , face1(make_shared<DummyFace>())
    , face2(make_shared<DummyFace>())
  {
    forwarder.addFace(face1);
    forwarder.addFace(face2);
  }

public:
  LimitedIo limitedIo;

  Forwarder forwarder;
  StrategyTester<S>& strategy;
  Fib& fib;
  Pit& pit;

  shared_ptr<Face> face1;
  shared_ptr<Face> face2;
};

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestStrategyNoRoute, BaseFixture)

template<typename S, typename C>
class Test
{
public:
  using Strategy = S;
  using Case = C;
};

template<typename S>
class EmptyNextHopList
{
public:
  Name
  getInterestName()
  {
    return "/P";
  }

  void
  insertFibEntry(StrategyNoRouteFixture<S>* fixture)
  {
    fixture->fib.insert(Name());
  }
};

template<typename S>
class NextHopIsDownstream
{
public:
  Name
  getInterestName()
  {
    return "/P";
  }

  void
  insertFibEntry(StrategyNoRouteFixture<S>* fixture)
  {
    fixture->fib.insert(Name()).first->addNextHop(*fixture->face1, 10);
  }
};

template<typename S>
class NextHopViolatesScope
{
public:
  Name
  getInterestName()
  {
    return "/localhop/P";
  }

  void
  insertFibEntry(StrategyNoRouteFixture<S>* fixture)
  {
    fixture->fib.insert("/localhop").first->addNextHop(*fixture->face2, 10);
    // face1 and face2 are both non-local; Interest from face1 cannot be forwarded to face2
  }
};

using Tests = boost::mpl::vector<
  Test<AsfStrategy, EmptyNextHopList<AsfStrategy>>,
  Test<AsfStrategy, NextHopIsDownstream<AsfStrategy>>,
  Test<AsfStrategy, NextHopViolatesScope<AsfStrategy>>,

  Test<BestRouteStrategy2, EmptyNextHopList<BestRouteStrategy2>>,
  Test<BestRouteStrategy2, NextHopIsDownstream<BestRouteStrategy2>>,
  Test<BestRouteStrategy2, NextHopViolatesScope<BestRouteStrategy2>>,

  Test<MulticastStrategy, EmptyNextHopList<MulticastStrategy>>,
  Test<MulticastStrategy, NextHopIsDownstream<MulticastStrategy>>,
  Test<MulticastStrategy, NextHopViolatesScope<MulticastStrategy>>
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(IncomingInterest, T, Tests,
                                 StrategyNoRouteFixture<typename T::Strategy>)
{
  typename T::Case scenario;
  scenario.insertFibEntry(this);

  shared_ptr<Interest> interest = makeInterest(scenario.getInterestName());
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->face1, *interest);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveInterest(*this->face1, *interest, pitEntry); },
    this->limitedIo, 2));

  BOOST_REQUIRE_EQUAL(this->strategy.rejectPendingInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(this->strategy.rejectPendingInterestHistory[0].pitInterest, pitEntry->getInterest());

  BOOST_REQUIRE_EQUAL(this->strategy.sendNackHistory.size(), 1);
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[0].pitInterest, pitEntry->getInterest());
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[0].outFaceId, this->face1->getId());
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory[0].header.getReason(), lp::NackReason::NO_ROUTE);
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyNoRoute
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
