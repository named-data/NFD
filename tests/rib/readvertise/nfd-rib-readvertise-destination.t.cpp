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

#include "rib/readvertise/nfd-rib-readvertise-destination.hpp"

#include "tests/test-common.hpp"
#include "tests/identity-management-fixture.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/security/signing-info.hpp>

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

class NfdRibReadvertiseDestinationFixture : public IdentityManagementTimeFixture
{
public:
  NfdRibReadvertiseDestinationFixture()
    : nSuccessCallbacks(0)
    , nFailureCallbacks(0)
    , face(getGlobalIoService(), m_keyChain, {true, false})
    , controller(face, m_keyChain)
    , dest(controller, Name("/localhost/nlsr"), rib)
    , successCallback([this] {
        nSuccessCallbacks++;
      })
    , failureCallback([this] (const std::string& str) {
        nFailureCallbacks++;
      })
  {
  }

public:
  uint32_t nSuccessCallbacks;
  uint32_t nFailureCallbacks;

protected:
  ndn::util::DummyClientFace face;
  ndn::nfd::Controller controller;
  Rib rib;
  NfdRibReadvertiseDestination dest;
  std::function<void()> successCallback;
  std::function<void(const std::string&)> failureCallback;
};

BOOST_AUTO_TEST_SUITE(Readvertise)
BOOST_FIXTURE_TEST_SUITE(TestNfdRibReadvertiseDestination, NfdRibReadvertiseDestinationFixture)

class AdvertiseSuccessScenario
{
public:
  ndn::nfd::ControlResponse
  makeResponse(const ControlParameters& sentCp)
  {
    ControlParameters response;

    response.setFaceId(1)
      .setName(sentCp.getName())
      .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT)
      .setCost(0)
      .setFlags(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

    ndn::nfd::ControlResponse responsePayload;
    responsePayload.setCode(200)
      .setText("Successfully registered.")
      .setBody(response.wireEncode());
    return responsePayload;
  }

  void
  checkCommandOutcome(NfdRibReadvertiseDestinationFixture* fixture)
  {
    BOOST_CHECK_EQUAL(fixture->nSuccessCallbacks, 1);
    BOOST_CHECK_EQUAL(fixture->nFailureCallbacks, 0);
  }
};

class AdvertiseFailureScenario
{
public:
  ndn::nfd::ControlResponse
  makeResponse(ControlParameters sentCp)
  {
    ndn::nfd::ControlResponse responsePayload(403, "Not Authenticated");
    return responsePayload;
  }

  void
  checkCommandOutcome(NfdRibReadvertiseDestinationFixture* fixture)
  {
    BOOST_CHECK_EQUAL(fixture->nFailureCallbacks, 1);
    BOOST_CHECK_EQUAL(fixture->nSuccessCallbacks, 0);
  }
};

using AdvertiseScenarios = boost::mpl::vector<AdvertiseSuccessScenario, AdvertiseFailureScenario>;

BOOST_AUTO_TEST_CASE_TEMPLATE(Advertise, Scenario, AdvertiseScenarios)
{
  Scenario scenario;
  Name prefix("/ndn/memphis/test");
  ReadvertisedRoute rr(prefix);
  const Name RIB_REGISTER_COMMAND_PREFIX("/localhost/nlsr/rib/register");

  dest.advertise(rr, successCallback, failureCallback);
  advanceClocks(time::milliseconds(100));

  // Retrieve the sent Interest to build the response
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  const Interest& sentInterest = face.sentInterests[0];
  BOOST_CHECK(RIB_REGISTER_COMMAND_PREFIX.isPrefixOf(sentInterest.getName()));

  // Parse the sent command Interest to check correctness.
  ControlParameters sentCp;
  BOOST_CHECK_NO_THROW(sentCp.wireDecode(sentInterest.getName().get(RIB_REGISTER_COMMAND_PREFIX.size()).blockFromValue()));
  BOOST_CHECK_EQUAL(sentCp.getOrigin(), ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(sentCp.getName(), prefix);

  ndn::nfd::ControlResponse responsePayload = scenario.makeResponse(sentCp);
  auto responseData = makeData(sentInterest.getName());
  responseData->setContent(responsePayload.wireEncode());
  face.receive(*responseData);
  this->advanceClocks(time::milliseconds(10));

  scenario.checkCommandOutcome(this);
}

class WithdrawSuccessScenario
{
public:
  ndn::nfd::ControlResponse
  makeResponse(const ControlParameters& sentCp)
  {
    ControlParameters response;

    response.setFaceId(1)
      .setName(sentCp.getName())
      .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT);

    ndn::nfd::ControlResponse responsePayload;
    responsePayload.setCode(200)
      .setText("Successfully removed")
      .setBody(response.wireEncode());

    return responsePayload;
  }

  void
  checkCommandOutcome(NfdRibReadvertiseDestinationFixture* fixture)
  {
    BOOST_CHECK_EQUAL(fixture->nSuccessCallbacks, 1);
    BOOST_CHECK_EQUAL(fixture->nFailureCallbacks, 0);
  }
};

class WithdrawFailureScenario
{
public:
  ndn::nfd::ControlResponse
  makeResponse(ControlParameters sentCp)
  {
    ndn::nfd::ControlResponse responsePayload(403, "Not authenticated");
    return responsePayload;
  }

  void
  checkCommandOutcome(NfdRibReadvertiseDestinationFixture* fixture)
  {
    BOOST_CHECK_EQUAL(fixture->nFailureCallbacks, 1);
    BOOST_CHECK_EQUAL(fixture->nSuccessCallbacks, 0);
  }
};

using WithdrawScenarios = boost::mpl::vector<WithdrawSuccessScenario, WithdrawFailureScenario>;

BOOST_AUTO_TEST_CASE_TEMPLATE(Withdraw, Scenario, WithdrawScenarios)
{
  Scenario scenario;
  Name prefix("/ndn/memphis/test");
  ReadvertisedRoute rr(prefix);
  const Name RIB_UNREGISTER_COMMAND_PREFIX("/localhost/nlsr/rib/unregister");

  dest.withdraw(rr, successCallback, failureCallback);
  this->advanceClocks(time::milliseconds(10));

  // Retrieve the sent Interest to build the response
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  const Interest& sentInterest = face.sentInterests[0];
  BOOST_CHECK(RIB_UNREGISTER_COMMAND_PREFIX.isPrefixOf(sentInterest.getName()));

  ControlParameters sentCp;
  BOOST_CHECK_NO_THROW(sentCp.wireDecode(sentInterest.getName().get(RIB_UNREGISTER_COMMAND_PREFIX.size()).blockFromValue()));
  BOOST_CHECK_EQUAL(sentCp.getOrigin(), ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(sentCp.getName(), prefix);

  ndn::nfd::ControlResponse responsePayload = scenario.makeResponse(sentCp);
  auto responseData = makeData(sentInterest.getName());
  responseData->setContent(responsePayload.wireEncode());

  face.receive(*responseData);
  this->advanceClocks(time::milliseconds(1));

  scenario.checkCommandOutcome(this);
}

BOOST_AUTO_TEST_CASE(DestinationAvailability)
{
  std::vector<bool> availabilityChangeHistory;
  Name commandPrefix("/localhost/nlsr");
  Route route;

  dest.afterAvailabilityChange.connect(
    std::bind(&std::vector<bool>::push_back, &availabilityChangeHistory, _1));
  BOOST_CHECK_EQUAL(dest.isAvailable(), false);

  rib.insert(commandPrefix, route);
  this->advanceClocks(time::milliseconds(100));
  BOOST_CHECK_EQUAL(dest.isAvailable(), true);
  BOOST_REQUIRE_EQUAL(availabilityChangeHistory.size(), 1);
  BOOST_CHECK_EQUAL(availabilityChangeHistory.back(), true);

  rib.erase(commandPrefix, route);
  this->advanceClocks(time::milliseconds(100));
  BOOST_CHECK_EQUAL(dest.isAvailable(), false);
  BOOST_REQUIRE_EQUAL(availabilityChangeHistory.size(), 2);
  BOOST_CHECK_EQUAL(availabilityChangeHistory.back(), false);
}

BOOST_AUTO_TEST_SUITE_END() // TestNfdRibReadvertiseDestination
BOOST_AUTO_TEST_SUITE_END() // Readvertise

} // namespace tests
} // namespace rib
} // namespace nfd
