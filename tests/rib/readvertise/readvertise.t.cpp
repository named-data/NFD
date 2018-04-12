/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "rib/readvertise/readvertise.hpp"

#include "tests/identity-management-fixture.hpp"

#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

class DummyReadvertisePolicy : public ReadvertisePolicy
{
public:
  optional<ReadvertiseAction>
  handleNewRoute(const RibRouteRef& route) const override
  {
    return this->decision;
  }

  time::milliseconds
  getRefreshInterval() const override
  {
    return time::seconds(60);
  }

public:
  optional<ReadvertiseAction> decision;
};

class DummyReadvertiseDestination : public ReadvertiseDestination
{
public:
  DummyReadvertiseDestination()
  {
    this->setAvailability(true);
  }

  void
  advertise(const ReadvertisedRoute& rr,
            std::function<void()> successCb,
            std::function<void(const std::string&)> failureCb) override
  {
    advertiseHistory.push_back({time::steady_clock::now(), rr.prefix});
    if (shouldSucceed) {
      successCb();
    }
    else {
      failureCb("FAKE-FAILURE");
    }
  }

  void
  withdraw(const ReadvertisedRoute& rr,
           std::function<void()> successCb,
           std::function<void(const std::string&)> failureCb) override
  {
    withdrawHistory.push_back({time::steady_clock::now(), rr.prefix});
    if (shouldSucceed) {
      successCb();
    }
    else {
      failureCb("FAKE-FAILURE");
    }
  }

  void
  setAvailability(bool isAvailable)
  {
    this->ReadvertiseDestination::setAvailability(isAvailable);
  }

public:
  struct HistoryEntry
  {
    time::steady_clock::TimePoint timestamp;
    Name prefix;
  };

  bool shouldSucceed = true;
  std::vector<HistoryEntry> advertiseHistory;
  std::vector<HistoryEntry> withdrawHistory;
};

class ReadvertiseFixture : public IdentityManagementTimeFixture
{
public:
  ReadvertiseFixture()
    : face(getGlobalIoService(), m_keyChain, {false, false})
    , controller(face, m_keyChain)
  {
    auto policyUnique = make_unique<DummyReadvertisePolicy>();
    policy = policyUnique.get();
    auto destinationUnique = make_unique<DummyReadvertiseDestination>();
    destination = destinationUnique.get();
    readvertise.reset(new Readvertise(rib, std::move(policyUnique), std::move(destinationUnique)));
  }

  void
  insertRoute(const Name& prefix, uint64_t faceId, ndn::nfd::RouteOrigin origin)
  {
    Route route;
    route.faceId = faceId;
    route.origin = origin;
    rib.insert(prefix, route);
    this->advanceClocks(time::milliseconds(6));
  }

  void
  eraseRoute(const Name& prefix, uint64_t faceId, ndn::nfd::RouteOrigin origin)
  {
    Route route;
    route.faceId = faceId;
    route.origin = origin;
    rib.erase(prefix, route);
    this->advanceClocks(time::milliseconds(6));
  }

  void
  setDestinationAvailability(bool isAvailable)
  {
    destination->setAvailability(isAvailable);
    this->advanceClocks(time::milliseconds(6));
  }

public:
  ndn::KeyChain m_keyChain;
  ndn::util::DummyClientFace face;
  ndn::nfd::Controller controller;
  DummyReadvertisePolicy* policy;
  DummyReadvertiseDestination* destination;
  Rib rib;
  unique_ptr<Readvertise> readvertise;
};

BOOST_AUTO_TEST_SUITE(Readvertise)
BOOST_FIXTURE_TEST_SUITE(TestReadvertise, ReadvertiseFixture)

BOOST_AUTO_TEST_CASE(AddRemoveRoute)
{
  policy->decision = ReadvertiseAction{"/A", ndn::security::SigningInfo()};

  // advertising /A
  this->insertRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(destination->advertiseHistory.size(), 1);
  BOOST_CHECK_EQUAL(destination->advertiseHistory.at(0).prefix, "/A");

  // /A is already advertised
  this->insertRoute("/A/2", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(destination->advertiseHistory.size(), 1);

  // refresh every 60 seconds
  destination->advertiseHistory.clear();
  this->advanceClocks(time::seconds(61), 5);
  BOOST_CHECK_EQUAL(destination->advertiseHistory.size(), 5);

  // /A is still needed by /A/2 route
  this->eraseRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(destination->withdrawHistory.size(), 0);

  // withdrawing /A
  this->eraseRoute("/A/2", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(destination->withdrawHistory.size(), 1);
  BOOST_CHECK_EQUAL(destination->withdrawHistory.at(0).prefix, "/A");
}

BOOST_AUTO_TEST_CASE(NoAdvertise)
{
  policy->decision = nullopt;

  this->insertRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  this->insertRoute("/A/2", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  this->eraseRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  this->eraseRoute("/A/2", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);

  BOOST_CHECK_EQUAL(destination->advertiseHistory.size(), 0);
  BOOST_CHECK_EQUAL(destination->withdrawHistory.size(), 0);
}

BOOST_AUTO_TEST_CASE(DestinationAvailability)
{
  this->setDestinationAvailability(false);

  policy->decision = ReadvertiseAction{"/A", ndn::security::SigningInfo()};
  this->insertRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  policy->decision = ReadvertiseAction{"/B", ndn::security::SigningInfo()};
  this->insertRoute("/B/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(destination->advertiseHistory.size(), 0);

  this->setDestinationAvailability(true);
  std::set<Name> advertisedPrefixes;
  boost::copy(destination->advertiseHistory | boost::adaptors::transformed(
                [] (const DummyReadvertiseDestination::HistoryEntry& he) { return he.prefix; }),
              std::inserter(advertisedPrefixes, advertisedPrefixes.end()));
  std::set<Name> expectedPrefixes{"/A", "/B"};
  BOOST_CHECK_EQUAL_COLLECTIONS(advertisedPrefixes.begin(), advertisedPrefixes.end(),
                                expectedPrefixes.begin(), expectedPrefixes.end());
  destination->advertiseHistory.clear();

  this->setDestinationAvailability(false);
  this->eraseRoute("/B/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(destination->withdrawHistory.size(), 0);

  this->setDestinationAvailability(true);
  BOOST_CHECK_EQUAL(destination->withdrawHistory.size(), 0);
  BOOST_CHECK_EQUAL(destination->advertiseHistory.size(), 1);
  BOOST_CHECK_EQUAL(destination->advertiseHistory.at(0).prefix, "/A");
}

BOOST_AUTO_TEST_CASE(AdvertiseRetryInterval)
{
  destination->shouldSucceed = false;

  policy->decision = ReadvertiseAction{"/A", ndn::security::SigningInfo()};
  this->insertRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);

  this->advanceClocks(time::seconds(10), time::seconds(3600));
  BOOST_REQUIRE_GT(destination->advertiseHistory.size(), 2);

  // destination->advertise keeps failing, so interval should increase
  using FloatInterval = time::duration<float, time::steady_clock::Duration::period>;
  FloatInterval initialInterval = destination->advertiseHistory[1].timestamp -
                                  destination->advertiseHistory[0].timestamp;
  FloatInterval lastInterval = initialInterval;
  for (size_t i = 2; i < destination->advertiseHistory.size(); ++i) {
    FloatInterval interval = destination->advertiseHistory[i].timestamp -
                             destination->advertiseHistory[i - 1].timestamp;
    BOOST_CHECK_CLOSE(interval.count(), (lastInterval * 2.0).count(), 10.0);
    lastInterval = interval;
  }

  destination->shouldSucceed = true;
  this->advanceClocks(time::seconds(3600));
  destination->advertiseHistory.clear();

  // destination->advertise has succeeded, retry interval should reset to initial
  destination->shouldSucceed = false;
  this->advanceClocks(time::seconds(10), time::seconds(300));
  BOOST_REQUIRE_GE(destination->advertiseHistory.size(), 2);
  FloatInterval restartInterval = destination->advertiseHistory[1].timestamp -
                                  destination->advertiseHistory[0].timestamp;
  BOOST_CHECK_CLOSE(restartInterval.count(), initialInterval.count(), 10.0);
}

BOOST_AUTO_TEST_CASE(ChangeDuringRetry)
{
  destination->shouldSucceed = false;
  policy->decision = ReadvertiseAction{"/A", ndn::security::SigningInfo()};
  this->insertRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  this->advanceClocks(time::seconds(10), time::seconds(300));
  BOOST_CHECK_GT(destination->advertiseHistory.size(), 0);
  BOOST_CHECK_EQUAL(destination->withdrawHistory.size(), 0);

  // destination->advertise has been failing, but we want to withdraw
  destination->advertiseHistory.clear();
  destination->withdrawHistory.clear();
  this->eraseRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  this->advanceClocks(time::seconds(10), time::seconds(300));
  BOOST_CHECK_EQUAL(destination->advertiseHistory.size(), 0); // don't try to advertise
  BOOST_CHECK_GT(destination->withdrawHistory.size(), 0); // try to withdraw

  // destination->withdraw has been failing, but we want to advertise
  destination->advertiseHistory.clear();
  destination->withdrawHistory.clear();
  this->insertRoute("/A/1", 1, ndn::nfd::ROUTE_ORIGIN_CLIENT);
  this->advanceClocks(time::seconds(10), time::seconds(300));
  BOOST_CHECK_GT(destination->advertiseHistory.size(), 0); // try to advertise
  BOOST_CHECK_EQUAL(destination->withdrawHistory.size(), 0); // don't try to withdraw
}

BOOST_AUTO_TEST_SUITE_END() // TestReadvertise
BOOST_AUTO_TEST_SUITE_END() // Readvertise

} // namespace tests
} // namespace rib
} // namespace nfd
