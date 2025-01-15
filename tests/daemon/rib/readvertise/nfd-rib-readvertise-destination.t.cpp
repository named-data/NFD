/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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
#include "tests/key-chain-fixture.hpp"
#include "tests/daemon/global-io-fixture.hpp"

#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/security/signing-info.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

#include <boost/mp11/list.hpp>

namespace nfd::tests {

using namespace nfd::rib;

class NfdRibReadvertiseDestinationFixture : public GlobalIoTimeFixture, public KeyChainFixture
{
protected:
  static inline const Name RIB_REGISTER_COMMAND_PREFIX = Name("/localhost/nlsr")
                                                         .append(ndn::nfd::RibRegisterCommand::getName());
  static inline const Name RIB_UNREGISTER_COMMAND_PREFIX = Name("/localhost/nlsr")
                                                           .append(ndn::nfd::RibUnregisterCommand::getName());

  ndn::DummyClientFace face{g_io, m_keyChain, {true, false}};
  ndn::nfd::Controller controller{face, m_keyChain};
  Rib rib;
  NfdRibReadvertiseDestination dest{controller, rib, ndn::nfd::CommandOptions().setPrefix("/localhost/nlsr")};

  std::function<void()> successCallback = [this] { nSuccessCallbacks++; };
  std::function<void(const std::string&)> failureCallback = [this] (auto&&) { nFailureCallbacks++; };

  int nSuccessCallbacks = 0;
  int nFailureCallbacks = 0;
};

BOOST_AUTO_TEST_SUITE(Rib)
BOOST_FIXTURE_TEST_SUITE(TestNfdRibReadvertiseDestination, NfdRibReadvertiseDestinationFixture)

struct AdvertiseSuccessScenario
{
  static ndn::nfd::ControlResponse
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

  static constexpr int nExpectedSuccessCalls = 1;
  static constexpr int nExpectedFailureCalls = 0;
};

struct AdvertiseFailureScenario
{
  static ndn::nfd::ControlResponse
  makeResponse(const ControlParameters& sentCp)
  {
    return ndn::nfd::ControlResponse(403, "Not Authenticated");
  }

  static constexpr int nExpectedSuccessCalls = 0;
  static constexpr int nExpectedFailureCalls = 1;
};

using AdvertiseScenarios = boost::mp11::mp_list<AdvertiseSuccessScenario, AdvertiseFailureScenario>;

BOOST_AUTO_TEST_CASE_TEMPLATE(Advertise, Scenario, AdvertiseScenarios)
{
  Name prefix("/ndn/memphis/test");
  ReadvertisedRoute rr(prefix);

  dest.advertise(rr, successCallback, failureCallback);
  advanceClocks(100_ms);

  // Retrieve the sent Interest to build the response
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  const Interest& sentInterest = face.sentInterests[0];
  BOOST_CHECK(RIB_REGISTER_COMMAND_PREFIX.isPrefixOf(sentInterest.getName()));

  // Parse the sent command Interest to check correctness.
  ControlParameters sentCp;
  BOOST_CHECK_NO_THROW(sentCp.wireDecode(sentInterest.getName().get(RIB_REGISTER_COMMAND_PREFIX.size())
                                         .blockFromValue()));
  BOOST_CHECK_EQUAL(sentCp.getOrigin(), ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(sentCp.getName(), prefix);

  ndn::nfd::ControlResponse responsePayload = Scenario::makeResponse(sentCp);
  auto responseData = makeData(sentInterest.getName());
  responseData->setContent(responsePayload.wireEncode());
  face.receive(*responseData);
  this->advanceClocks(10_ms);

  BOOST_TEST(nSuccessCallbacks == Scenario::nExpectedSuccessCalls);
  BOOST_TEST(nFailureCallbacks == Scenario::nExpectedFailureCalls);
}

struct WithdrawSuccessScenario
{
  static ndn::nfd::ControlResponse
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

  static constexpr int nExpectedSuccessCalls = 1;
  static constexpr int nExpectedFailureCalls = 0;
};

struct WithdrawFailureScenario
{
  static ndn::nfd::ControlResponse
  makeResponse(const ControlParameters& sentCp)
  {
    return ndn::nfd::ControlResponse(403, "Not authenticated");
  }

  static constexpr int nExpectedSuccessCalls = 0;
  static constexpr int nExpectedFailureCalls = 1;
};

using WithdrawScenarios = boost::mp11::mp_list<WithdrawSuccessScenario, WithdrawFailureScenario>;

BOOST_AUTO_TEST_CASE_TEMPLATE(Withdraw, Scenario, WithdrawScenarios)
{
  Name prefix("/ndn/memphis/test");
  ReadvertisedRoute rr(prefix);

  dest.withdraw(rr, successCallback, failureCallback);
  this->advanceClocks(10_ms);

  // Retrieve the sent Interest to build the response
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  const Interest& sentInterest = face.sentInterests[0];
  BOOST_CHECK(RIB_UNREGISTER_COMMAND_PREFIX.isPrefixOf(sentInterest.getName()));

  ControlParameters sentCp;
  BOOST_CHECK_NO_THROW(sentCp.wireDecode(sentInterest.getName().get(RIB_UNREGISTER_COMMAND_PREFIX.size())
                                         .blockFromValue()));
  BOOST_CHECK_EQUAL(sentCp.getOrigin(), ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(sentCp.getName(), prefix);

  ndn::nfd::ControlResponse responsePayload = Scenario::makeResponse(sentCp);
  auto responseData = makeData(sentInterest.getName());
  responseData->setContent(responsePayload.wireEncode());

  face.receive(*responseData);
  this->advanceClocks(1_ms);

  BOOST_TEST(nSuccessCallbacks == Scenario::nExpectedSuccessCalls);
  BOOST_TEST(nFailureCallbacks == Scenario::nExpectedFailureCalls);
}

BOOST_AUTO_TEST_CASE(DestinationAvailability)
{
  std::vector<bool> availabilityChangeHistory;
  Name commandPrefix("/localhost/nlsr");
  Route route;

  dest.afterAvailabilityChange.connect([&] (bool val) { availabilityChangeHistory.push_back(val); });
  BOOST_CHECK_EQUAL(dest.isAvailable(), false);

  rib.insert(commandPrefix, route);
  this->advanceClocks(100_ms);
  BOOST_CHECK_EQUAL(dest.isAvailable(), true);
  BOOST_REQUIRE_EQUAL(availabilityChangeHistory.size(), 1);
  BOOST_CHECK_EQUAL(availabilityChangeHistory.back(), true);

  rib.erase(commandPrefix, route);
  this->advanceClocks(100_ms);
  BOOST_CHECK_EQUAL(dest.isAvailable(), false);
  BOOST_REQUIRE_EQUAL(availabilityChangeHistory.size(), 2);
  BOOST_CHECK_EQUAL(availabilityChangeHistory.back(), false);
}

BOOST_AUTO_TEST_SUITE_END() // TestNfdRibReadvertiseDestination
BOOST_AUTO_TEST_SUITE_END() // Rib

} // namespace nfd::tests
