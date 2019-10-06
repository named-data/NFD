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

/** \file
 *  This test suite tests localhost and localhop scope control in strategies.
 */

// Strategies implementing namespace-based scope control, sorted alphabetically.
#include "fw/access-strategy.hpp"
#include "fw/asf-strategy.hpp"
#include "fw/best-route-strategy.hpp"
#include "fw/best-route-strategy2.hpp"
#include "fw/multicast-strategy.hpp"
#include "fw/ncc-strategy.hpp"
#include "fw/random-strategy.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "choose-strategy.hpp"
#include "strategy-tester.hpp"

#include <boost/mpl/copy_if.hpp>
#include <boost/mpl/vector.hpp>

namespace nfd {
namespace fw {
namespace tests {

template<typename S>
class StrategyScopeControlFixture : public GlobalIoTimeFixture
{
public:
  StrategyScopeControlFixture()
    : limitedIo(this)
    , forwarder(faceTable)
    , strategy(choose<StrategyTester<S>>(forwarder))
    , fib(forwarder.getFib())
    , pit(forwarder.getPit())
    , nonLocalFace1(make_shared<DummyFace>("dummy://1", "dummy://1", ndn::nfd::FACE_SCOPE_NON_LOCAL))
    , nonLocalFace2(make_shared<DummyFace>("dummy://2", "dummy://2", ndn::nfd::FACE_SCOPE_NON_LOCAL))
    , localFace3(make_shared<DummyFace>("dummy://3", "dummy://3", ndn::nfd::FACE_SCOPE_LOCAL))
    , localFace4(make_shared<DummyFace>("dummy://4", "dummy://4", ndn::nfd::FACE_SCOPE_LOCAL))
  {
    faceTable.add(nonLocalFace1);
    faceTable.add(nonLocalFace2);
    faceTable.add(localFace3);
    faceTable.add(localFace4);
  }

public:
  LimitedIo limitedIo;

  FaceTable faceTable;
  Forwarder forwarder;
  StrategyTester<S>& strategy;
  Fib& fib;
  Pit& pit;

  shared_ptr<Face> nonLocalFace1;
  shared_ptr<Face> nonLocalFace2;
  shared_ptr<Face> localFace3;
  shared_ptr<Face> localFace4;
};

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_AUTO_TEST_SUITE(TestStrategyScopeControl)

template<typename S, bool WillSendNackNoRoute, bool CanProcessNack>
class Test
{
public:
  using Strategy = S;

  static bool
  willSendNackNoRoute()
  {
    return WillSendNackNoRoute;
  }

  static bool
  canProcessNack()
  {
    return CanProcessNack;
  }
};

using Tests = boost::mpl::vector<
  Test<AccessStrategy, false, false>,
  Test<AsfStrategy, true, false>,
  Test<BestRouteStrategy, false, false>,
  Test<BestRouteStrategy2, true, true>,
  Test<MulticastStrategy, true, true>,
  Test<NccStrategy, false, false>,
  Test<RandomStrategy, true, true>
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(LocalhostInterestToLocal,
                                 T, Tests, StrategyScopeControlFixture<typename T::Strategy>)
{
  fib::Entry* fibEntry = this->fib.insert("/localhost/A").first;
  this->fib.addOrUpdateNextHop(*fibEntry, *this->localFace4, 10);

  auto interest = makeInterest("/localhost/A/1");
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->localFace3, *interest);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveInterest(FaceEndpoint(*this->localFace3, 0), *interest, pitEntry); },
    this->limitedIo));

  BOOST_CHECK_EQUAL(this->strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(this->strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.size(), 0);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(LocalhostInterestToNonLocal,
                                 T, Tests, StrategyScopeControlFixture<typename T::Strategy>)
{
  fib::Entry* fibEntry = this->fib.insert("/localhost/A").first;
  this->fib.addOrUpdateNextHop(*fibEntry, *this->nonLocalFace2, 10);

  auto interest = makeInterest("/localhost/A/1");
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->localFace3, *interest);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveInterest(FaceEndpoint(*this->localFace3, 0), *interest, pitEntry); },
    this->limitedIo, 1 + T::willSendNackNoRoute()));

  BOOST_CHECK_EQUAL(this->strategy.sendInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(this->strategy.rejectPendingInterestHistory.size(), 1);
  if (T::willSendNackNoRoute()) {
    BOOST_REQUIRE_EQUAL(this->strategy.sendNackHistory.size(), 1);
    BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.back().header.getReason(), lp::NackReason::NO_ROUTE);
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(LocalhostInterestToLocalAndNonLocal,
                                 T, Tests, StrategyScopeControlFixture<typename T::Strategy>)
{
  fib::Entry* fibEntry = this->fib.insert("/localhost/A").first;
  this->fib.addOrUpdateNextHop(*fibEntry, *this->nonLocalFace2, 10);
  this->fib.addOrUpdateNextHop(*fibEntry, *this->localFace4, 20);

  auto interest = makeInterest("/localhost/A/1");
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->localFace3, *interest);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveInterest(FaceEndpoint(*this->localFace3, 0), *interest, pitEntry); },
    this->limitedIo));

  BOOST_REQUIRE_EQUAL(this->strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(this->strategy.sendInterestHistory.back().outFaceId, this->localFace4->getId());
  BOOST_CHECK_EQUAL(this->strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.size(), 0);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(LocalhopInterestToNonLocal,
                                 T, Tests, StrategyScopeControlFixture<typename T::Strategy>)
{
  fib::Entry* fibEntry = this->fib.insert("/localhop/A").first;
  this->fib.addOrUpdateNextHop(*fibEntry, *this->nonLocalFace2, 10);

  auto interest = makeInterest("/localhop/A/1");
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->nonLocalFace1, *interest);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveInterest(FaceEndpoint(*this->nonLocalFace1, 0), *interest, pitEntry); },
    this->limitedIo, 1 + T::willSendNackNoRoute()));

  BOOST_CHECK_EQUAL(this->strategy.sendInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(this->strategy.rejectPendingInterestHistory.size(), 1);
  if (T::willSendNackNoRoute()) {
    BOOST_REQUIRE_EQUAL(this->strategy.sendNackHistory.size(), 1);
    BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.back().header.getReason(), lp::NackReason::NO_ROUTE);
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(LocalhopInterestToNonLocalAndLocal,
                                 T, Tests, StrategyScopeControlFixture<typename T::Strategy>)
{
  fib::Entry* fibEntry = this->fib.insert("/localhop/A").first;
  this->fib.addOrUpdateNextHop(*fibEntry, *this->nonLocalFace2, 10);
  this->fib.addOrUpdateNextHop(*fibEntry, *this->localFace4, 20);

  auto interest = makeInterest("/localhop/A/1");
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->nonLocalFace1, *interest);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveInterest(FaceEndpoint(*this->nonLocalFace1, 0), *interest, pitEntry); },
    this->limitedIo));

  BOOST_REQUIRE_EQUAL(this->strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(this->strategy.sendInterestHistory.back().outFaceId, this->localFace4->getId());
  BOOST_CHECK_EQUAL(this->strategy.rejectPendingInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.size(), 0);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(LocalhostNackToNonLocal,
                                 T, Tests, StrategyScopeControlFixture<typename T::Strategy>)
{
  fib::Entry* fibEntry = this->fib.insert("/localhost/A").first;
  this->fib.addOrUpdateNextHop(*fibEntry, *this->localFace4, 10);
  this->fib.addOrUpdateNextHop(*fibEntry, *this->nonLocalFace2, 20);

  auto interest = makeInterest("/localhost/A/1", false, nullopt, 1460);
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->localFace3, *interest);
  lp::Nack nack = makeNack(*interest, lp::NackReason::NO_ROUTE);
  pitEntry->insertOrUpdateOutRecord(*this->localFace4, *interest)->setIncomingNack(nack);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveNack(FaceEndpoint(*this->localFace4, 0), nack, pitEntry); },
    this->limitedIo, T::canProcessNack()));

  BOOST_CHECK_EQUAL(this->strategy.sendInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(this->strategy.rejectPendingInterestHistory.size(), 0);
  if (T::canProcessNack()) {
    BOOST_REQUIRE_EQUAL(this->strategy.sendNackHistory.size(), 1);
    BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.back().header.getReason(), lp::NackReason::NO_ROUTE);
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(LocalhopNackToNonLocal,
                                 T, Tests, StrategyScopeControlFixture<typename T::Strategy>)
{
  fib::Entry* fibEntry = this->fib.insert("/localhop/A").first;
  this->fib.addOrUpdateNextHop(*fibEntry, *this->localFace4, 10);
  this->fib.addOrUpdateNextHop(*fibEntry, *this->nonLocalFace2, 20);

  auto interest = makeInterest("/localhop/A/1", 1377);
  shared_ptr<pit::Entry> pitEntry = this->pit.insert(*interest).first;
  pitEntry->insertOrUpdateInRecord(*this->nonLocalFace1, *interest);
  lp::Nack nack = makeNack(*interest, lp::NackReason::NO_ROUTE);
  pitEntry->insertOrUpdateOutRecord(*this->localFace4, *interest)->setIncomingNack(nack);

  BOOST_REQUIRE(this->strategy.waitForAction(
    [&] { this->strategy.afterReceiveNack(FaceEndpoint(*this->localFace4, 0), nack, pitEntry); },
    this->limitedIo, T::canProcessNack()));

  BOOST_CHECK_EQUAL(this->strategy.sendInterestHistory.size(), 0);
  BOOST_CHECK_EQUAL(this->strategy.rejectPendingInterestHistory.size(), 0);
  if (T::canProcessNack()) {
    BOOST_REQUIRE_EQUAL(this->strategy.sendNackHistory.size(), 1);
    BOOST_CHECK_EQUAL(this->strategy.sendNackHistory.back().header.getReason(), lp::NackReason::NO_ROUTE);
  }
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyScopeControl
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
