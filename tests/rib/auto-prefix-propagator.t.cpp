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

#include "rib/auto-prefix-propagator.hpp"

#include <ndn-cxx/security/pib/pib.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

#include "tests/identity-management-fixture.hpp"

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

const Name TEST_LINK_LOCAL_NFD_PREFIX("/localhop/nfd");
const time::milliseconds TEST_PREFIX_PROPAGATION_TIMEOUT(1000);

class AutoPrefixPropagatorFixture : public IdentityManagementTimeFixture
{
public:
  AutoPrefixPropagatorFixture()
    : m_face(getGlobalIoService(), m_keyChain, {true, true})
    , m_controller(m_face, m_keyChain)
    , m_propagator(m_controller, m_keyChain, m_rib)
    , m_requests(m_face.sentInterests)
    , m_entries(m_propagator.m_propagatedEntries)
  {
    m_propagator.enable();
    m_propagator.m_controlParameters
      .setCost(15)
      .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT)// set origin to client.
      .setFaceId(0);// the remote hub will take the input face as the faceId.
    m_propagator.m_commandOptions
      .setPrefix(TEST_LINK_LOCAL_NFD_PREFIX)
      .setTimeout(TEST_PREFIX_PROPAGATION_TIMEOUT);
  }

public: // helpers for test
  bool
  insertEntryToRib(const Name& name, const uint64_t& faceId = 0)
  {
    if (m_rib.find(name) != m_rib.end()) {
      BOOST_TEST_MESSAGE("RIB entry already exists: " + name.toUri());
      return false;
    }

    Route route;
    route.faceId = faceId;
    m_rib.insert(name, route);
    advanceClocks(time::milliseconds(1));

    return m_rib.find(name) != m_rib.end(); // return whether afterInserEntry will be triggered
  }

  bool
  eraseEntryFromRib(const Name& name)
  {
    if (m_rib.find(name) == m_rib.end()) {
      BOOST_TEST_MESSAGE("RIB entry does not exist: " + name.toUri());
      return false;
    }

    std::vector<Route> routeList;
    std::copy(m_rib.find(name)->second->begin(), m_rib.find(name)->second->end(),
              std::back_inserter(routeList));
    for (auto&& route : routeList) {
      m_rib.erase(name, route);
    }
    advanceClocks(time::milliseconds(1));

    return m_rib.find(name) == m_rib.end(); // return whether afterEraseEntry will be triggered
  }

  void
  connectToHub()
  {
    insertEntryToRib("/localhop/nfd");
  }

  void
  disconnectFromHub()
  {
    eraseEntryFromRib("/localhop/nfd");
  }

public: // helpers for check
  enum class CheckRequestResult {
    OK,
    OUT_OF_BOUNDARY,
    INVALID_N_COMPONENTS,
    WRONG_COMMAND_PREFIX,
    WRONG_VERB,
    INVALID_PARAMETERS,
    WRONG_REGISTERING_PREFIX
  };

  /**
   * @brief check a request at specified index
   *
   * @param idx the index of the specified request in m_requests
   * @param verb the expected verb of request
   * @param registeringPrefix the expected registering prefix of request

   * @retval OK the specified request has a valid name, the right verb, a valid ControlParameters
   *            and the right registering prefix
   * @retval OUT_OF_BOUNDARY the specified index out of boundary
   * @retval INVALID_N_COMPONENTS the number of components of the request name is invalid
   * @retval WRONG_COMMAND_PREFIX the command prefix of the request is wrong
   * @retval WRONG_VERB the command verb of the request is wrong
   * @retval INVALID_PARAMETERS no valid parameters can be decoded from the request's name
   * @retval WRONG_REGISTERING_PREFIX the registering prefix of the request is wrong
   */
  CheckRequestResult
  checkRequest(size_t idx, const std::string& verb, const Name& registeringPrefix)
  {
    Name requestName;
    try {
      requestName = m_requests.at(idx).getName();
    }
    catch (const std::out_of_range&) {
      return CheckRequestResult::OUT_OF_BOUNDARY;
    }

    if (requestName.size() < 5) {
      return CheckRequestResult::INVALID_N_COMPONENTS;
    }

    if (requestName.getPrefix(2) != TEST_LINK_LOCAL_NFD_PREFIX) {
      return CheckRequestResult::WRONG_COMMAND_PREFIX;
    }

    if (requestName.get(3) != Name::Component(verb)) {
      return CheckRequestResult::WRONG_VERB;
    }

    ControlParameters parameters;
    try {
      parameters.wireDecode(requestName.get(4).blockFromValue());
    }
    catch (const tlv::Error&) {
      return CheckRequestResult::INVALID_PARAMETERS;
    }

    if (parameters.getName() != registeringPrefix) {
      return CheckRequestResult::WRONG_REGISTERING_PREFIX;
    }

    return CheckRequestResult::OK;
  }

protected:
  ndn::util::DummyClientFace m_face;
  ndn::nfd::Controller m_controller;
  Rib m_rib;
  AutoPrefixPropagator m_propagator;
  std::vector<Interest>& m_requests; ///< store sent out requests
  AutoPrefixPropagator::PropagatedEntryList& m_entries; ///< store propagated entries
};

std::ostream&
operator<<(std::ostream &os, const AutoPrefixPropagatorFixture::CheckRequestResult& result)
{
  switch (result) {
  case AutoPrefixPropagatorFixture::CheckRequestResult::OK:
    os << "OK";
    break;
  case AutoPrefixPropagatorFixture::CheckRequestResult::OUT_OF_BOUNDARY:
    os << "OUT_OF_BOUNDARY";
    break;
  case AutoPrefixPropagatorFixture::CheckRequestResult::INVALID_N_COMPONENTS:
    os << "INVALID_N_COMPONENTS";
    break;
  case AutoPrefixPropagatorFixture::CheckRequestResult::WRONG_COMMAND_PREFIX:
    os << "WRONG_COMMAND_PREFIX";
    break;
  case AutoPrefixPropagatorFixture::CheckRequestResult::WRONG_VERB:
    os << "WRONG_VERB";
    break;
  case AutoPrefixPropagatorFixture::CheckRequestResult::INVALID_PARAMETERS:
    os << "INVALID_PARAMETERS";
    break;
  case AutoPrefixPropagatorFixture::CheckRequestResult::WRONG_REGISTERING_PREFIX:
    os << "WRONG_REGISTERING_PREFIX";
    break;
  default:
    break;
  }

  return os;
}

BOOST_FIXTURE_TEST_SUITE(TestAutoPrefixPropagator, AutoPrefixPropagatorFixture)

BOOST_AUTO_TEST_CASE(EnableDisable)
{
  connectToHub();
  BOOST_REQUIRE(addIdentity("/test/A"));

  auto testPropagateRevokeBasic = [this] () -> bool {
    m_propagator.m_propagatedEntries.clear();

    if (!insertEntryToRib("/test/A/app")) {
      return false;
    }
    m_entries["/test/A"].succeed(nullptr);
    if (!eraseEntryFromRib("/test/A/app")) {
      return false;
    }
    return true;
  };

  m_propagator.disable();
  BOOST_REQUIRE(testPropagateRevokeBasic());
  BOOST_CHECK(m_requests.empty());

  m_propagator.enable();
  BOOST_REQUIRE(testPropagateRevokeBasic());
  BOOST_REQUIRE_EQUAL(m_requests.size(), 2);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/A"), CheckRequestResult::OK);
  BOOST_CHECK_EQUAL(checkRequest(1, "unregister", "/test/A"), CheckRequestResult::OK);
}

BOOST_AUTO_TEST_CASE(LoadConfiguration)
{
  ConfigFile config;
  config.addSectionHandler("auto_prefix_propagate",
                           bind(&AutoPrefixPropagator::loadConfig, &m_propagator, _1));

  const std::string CONFIG_STRING =
    "auto_prefix_propagate\n"
    "{\n"
    "  cost 11\n"
    "  timeout 22\n"
    "  refresh_interval 33\n"
    "  base_retry_wait 44\n"
    "  max_retry_wait 55\n"
    "}";
  config.parse(CONFIG_STRING, true, "test-auto-prefix-propagator");

  BOOST_CHECK_EQUAL(m_propagator.m_controlParameters.getCost(), 11);
  BOOST_CHECK_EQUAL(m_propagator.m_controlParameters.getOrigin(), ndn::nfd::ROUTE_ORIGIN_CLIENT);
  BOOST_CHECK_EQUAL(m_propagator.m_controlParameters.getFaceId(), 0);

  BOOST_CHECK_EQUAL(m_propagator.m_commandOptions.getPrefix(), TEST_LINK_LOCAL_NFD_PREFIX);
  BOOST_CHECK_EQUAL(m_propagator.m_commandOptions.getTimeout(), time::milliseconds(22));

  BOOST_CHECK_EQUAL(m_propagator.m_refreshInterval, time::seconds(33));
  BOOST_CHECK_EQUAL(m_propagator.m_baseRetryWait, time::seconds(44));
  BOOST_CHECK_EQUAL(m_propagator.m_maxRetryWait, time::seconds(55));
}

BOOST_AUTO_TEST_CASE(GetPrefixPropagationParameters)
{
  BOOST_REQUIRE(addIdentity("/test/A"));
  BOOST_REQUIRE(addIdentity("/test/A/B"));
  BOOST_REQUIRE(addIdentity("/test/C/nrd"));

  auto parameters1 = m_propagator.getPrefixPropagationParameters("/none/A/B/app");
  auto parameters2 = m_propagator.getPrefixPropagationParameters("/test/A/B/app");
  auto parameters3 = m_propagator.getPrefixPropagationParameters("/test/C/D/app");

  BOOST_CHECK(!parameters1.isValid);

  BOOST_CHECK(parameters2.isValid);
  BOOST_CHECK_EQUAL(parameters2.parameters.getName(), "/test/A");
  BOOST_CHECK_EQUAL(parameters2.options.getSigningInfo().getSignerName(), "/test/A");

  BOOST_CHECK(parameters3.isValid);
  BOOST_CHECK_EQUAL(parameters3.parameters.getName(), "/test/C");
  BOOST_CHECK_EQUAL(parameters3.options.getSigningInfo().getSignerName(), "/test/C/nrd");
}

BOOST_AUTO_TEST_CASE(CheckCurrentPropagatedPrefix)
{
  BOOST_REQUIRE(addIdentity("/test/A"));
  BOOST_REQUIRE(addIdentity("/test/B/nrd"));
  BOOST_REQUIRE(addIdentity("/test/A/B"));

  BOOST_CHECK(!m_propagator.doesCurrentPropagatedPrefixWork("/test/E")); // does not exist
  BOOST_CHECK(!m_propagator.doesCurrentPropagatedPrefixWork("/test/A/B")); // has a better option
  BOOST_CHECK(m_propagator.doesCurrentPropagatedPrefixWork("/test/A"));
  BOOST_CHECK(m_propagator.doesCurrentPropagatedPrefixWork("/test/B"));
}

BOOST_AUTO_TEST_CASE(RedoPropagation)
{
  connectToHub();
  BOOST_REQUIRE(addIdentity("/test/A"));
  BOOST_REQUIRE(addIdentity("/test/B"));
  BOOST_REQUIRE(addIdentity("/test/B/C"));
  BOOST_REQUIRE(insertEntryToRib("/test/A/app"));
  BOOST_REQUIRE(insertEntryToRib("/test/B/C/app"));
  BOOST_REQUIRE(insertEntryToRib("/test/B/D/app"));

  auto testRedoPropagation = [this] (const Name& signingIdentity) {
    m_requests.clear();

    m_entries[signingIdentity].setSigningIdentity(signingIdentity);

    auto parameters = m_propagator.m_controlParameters;
    auto options = m_propagator.m_commandOptions;
    ndn::security::SigningInfo info(ndn::security::SigningInfo::SIGNER_TYPE_ID, signingIdentity);
    m_propagator.redoPropagation(m_entries.find(signingIdentity),
                                  parameters.setName(signingIdentity),
                                  options.setSigningInfo(info),
                                  time::seconds(0));
    advanceClocks(time::milliseconds(1));
  };

  testRedoPropagation("/test/A"); // current propagated prefix still works
  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/A"), CheckRequestResult::OK);
  BOOST_CHECK(m_entries.find("test/A") != m_entries.end());

  m_keyChain.deleteIdentity(m_keyChain.getPib().getIdentity("/test/B"));
  testRedoPropagation("/test/B"); // signingIdentity no longer exists
  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/B/C"), CheckRequestResult::OK);
  BOOST_CHECK(m_entries.find("/test/B") == m_entries.end());
  BOOST_CHECK(m_entries.find("/test/B/C") != m_entries.end());

  testRedoPropagation("/test/B"); // no alternative identity
  BOOST_CHECK(m_requests.empty());
  BOOST_CHECK(m_entries.find("/test/B") == m_entries.end());

  m_entries["/test/B/C"].succeed(nullptr);
  testRedoPropagation("/test/B"); // alternative identity has been propagated
  BOOST_CHECK(m_requests.empty());
  BOOST_CHECK(m_entries.find("/test/B") == m_entries.end());

  BOOST_REQUIRE(addIdentity("/test/B/"));
  testRedoPropagation("/test/B/C"); // better option exists: /test/B
  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/B"), CheckRequestResult::OK);
  BOOST_CHECK(m_entries.find("test/B/C") == m_entries.end());
}

BOOST_AUTO_TEST_SUITE(PropagateRevokeSemantics)

BOOST_AUTO_TEST_CASE(Basic)
{
  connectToHub(); // ensure connectivity to the hub
  BOOST_REQUIRE(addIdentity("/test/A"));

  BOOST_REQUIRE(insertEntryToRib("/test/A/app")); // ensure afterInsertEntry signal emitted
  m_entries["/test/A"].succeed(nullptr); // ensure there is a valid entry inserted
  BOOST_REQUIRE(eraseEntryFromRib("/test/A/app")); // ensure afterEraseEntry signal emitted

  BOOST_REQUIRE_EQUAL(m_requests.size(), 2);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/A"), CheckRequestResult::OK);
  BOOST_CHECK_EQUAL(checkRequest(1, "unregister", "/test/A"), CheckRequestResult::OK);
}

BOOST_AUTO_TEST_CASE(LocalPrefix)
{
  connectToHub();
  BOOST_REQUIRE(addIdentity("/localhost/A"));

  BOOST_REQUIRE(insertEntryToRib("/localhost/A/app"));
  BOOST_CHECK(m_requests.empty());

  m_propagator.m_propagatedEntries["/localhost/A"].succeed(nullptr);
  BOOST_REQUIRE(eraseEntryFromRib("/localhost/A/app"));
  BOOST_CHECK(m_requests.empty());
}

BOOST_AUTO_TEST_CASE(InvalidPropagationParameters)
{
  connectToHub();

  // no identity can be found
  BOOST_CHECK(!m_propagator.getPrefixPropagationParameters("/test/A/app").isValid);

  BOOST_REQUIRE(insertEntryToRib("/test/A/app"));
  BOOST_CHECK(m_requests.empty());

  m_entries["/test/A"].succeed(nullptr);
  BOOST_REQUIRE(eraseEntryFromRib("/test/A/app"));
  BOOST_CHECK(m_requests.empty());
}

BOOST_AUTO_TEST_CASE(NoHubConnectivity)
{
  BOOST_REQUIRE(addIdentity("/test/A"));

  BOOST_REQUIRE(insertEntryToRib("/test/A/app"));
  BOOST_CHECK(m_requests.empty());

  m_entries["/test/A"].succeed(nullptr);
  BOOST_REQUIRE(eraseEntryFromRib("/test/A/app"));
  BOOST_CHECK(m_requests.empty());
}

BOOST_AUTO_TEST_CASE(ProcessSuspendedEntries)
{
  BOOST_REQUIRE(addIdentity("/test/A"));

  BOOST_REQUIRE(insertEntryToRib("/test/A/app1"));
  BOOST_REQUIRE(insertEntryToRib("/test/A/app2"));
  BOOST_CHECK(m_requests.empty()); // no propagation because no hub is connected
  BOOST_CHECK_EQUAL(m_entries.size(), 1); // /test/A was suspended

  BOOST_REQUIRE(eraseEntryFromRib("/test/A/app2"));
  BOOST_CHECK_EQUAL(m_entries.size(), 1); // /test/A was kept for app1

  // repeat the scenario that leads to BUG 3362.
  // ensure there is no improper assertion.
  BOOST_REQUIRE(insertEntryToRib("/test/A/app2"));
}

BOOST_AUTO_TEST_CASE(PropagatedEntryExists)
{
  connectToHub();
  BOOST_REQUIRE(addIdentity("/test/A"));
  BOOST_REQUIRE(insertEntryToRib("/test/A/app1"));

  m_requests.clear();
  BOOST_REQUIRE(insertEntryToRib("/test/A/app2")); // /test/A has been propagated by /test/A/app1
  BOOST_CHECK(m_requests.empty());
}

BOOST_AUTO_TEST_CASE(PropagatedEntryShouldKeep)
{
  connectToHub();
  BOOST_REQUIRE(addIdentity("/test/A"));
  BOOST_REQUIRE(insertEntryToRib("/test/A/app1"));
  BOOST_REQUIRE(insertEntryToRib("/test/A/app2"));

  m_requests.clear();
  BOOST_REQUIRE(eraseEntryFromRib("/test/A/app2")); // /test/A should be kept for /test/A/app1
  BOOST_CHECK(m_requests.empty());
}

BOOST_AUTO_TEST_CASE(BackOffRetryPolicy)
{
  m_propagator.m_commandOptions.setTimeout(time::milliseconds(1));
  m_propagator.m_baseRetryWait = time::seconds(1);
  m_propagator.m_maxRetryWait = time::seconds(2);

  connectToHub();
  BOOST_REQUIRE(addIdentity("/test/A"));
  BOOST_REQUIRE(insertEntryToRib("/test/A/app"));

  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/A"), CheckRequestResult::OK);

  advanceClocks(time::milliseconds(10), time::milliseconds(1050)); // wait for the 1st retry
  BOOST_REQUIRE_EQUAL(m_requests.size(), 2);
  BOOST_CHECK_EQUAL(checkRequest(1, "register", "/test/A"), CheckRequestResult::OK);

  advanceClocks(time::milliseconds(10), time::milliseconds(2050)); // wait for the 2nd retry, 2 times
  BOOST_REQUIRE_EQUAL(m_requests.size(), 3);
  BOOST_CHECK_EQUAL(checkRequest(2, "register", "/test/A"), CheckRequestResult::OK);

  advanceClocks(time::milliseconds(10), time::milliseconds(2050)); // wait for the 3rd retry, reach the upper bound
  BOOST_REQUIRE_EQUAL(m_requests.size(), 4);
  BOOST_CHECK_EQUAL(checkRequest(3, "register", "/test/A"), CheckRequestResult::OK);
}

BOOST_AUTO_TEST_SUITE_END() // PropagateRevokeSemantics

BOOST_AUTO_TEST_SUITE(PropagatedEntryStateChanges)

BOOST_AUTO_TEST_CASE(AfterRibInsert)
{
  BOOST_REQUIRE(addIdentity("/test/A"));

  auto testAfterRibInsert = [this] (const Name& ribEntryPrefix) {
    m_requests.clear();
    m_propagator.m_propagatedEntries.clear(); // ensure entry does not exist

    auto propagateParameters = m_propagator.getPrefixPropagationParameters(ribEntryPrefix);
    m_propagator.afterRibInsert(propagateParameters.parameters, propagateParameters.options);
    advanceClocks(time::milliseconds(1));
  };

  testAfterRibInsert("/test/A/app1");
  BOOST_CHECK(m_requests.empty()); // no connectivity now
  BOOST_CHECK(m_entries.find("/test/A") != m_entries.end());

  connectToHub();
  testAfterRibInsert("/test/A/app2");
  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/A"), CheckRequestResult::OK);
  BOOST_CHECK(m_entries.find("/test/A") != m_entries.end());
}

BOOST_AUTO_TEST_CASE(AfterRibErase)
{
  BOOST_REQUIRE(addIdentity("/test/A"));

  auto testAfterRibInsert = [this] (const Name& localUnregPrefix) {
    m_requests.clear();

    auto propagateParameters = m_propagator.getPrefixPropagationParameters(localUnregPrefix);
    m_propagator.afterRibErase(propagateParameters.parameters.unsetCost(),
                               propagateParameters.options);
    advanceClocks(time::milliseconds(1));
  };

  m_entries["/test/A"].succeed(nullptr);
  testAfterRibInsert("/test/A/app");
  BOOST_CHECK(m_requests.empty()); // no connectivity
  BOOST_CHECK(m_entries.find("/test/A") == m_entries.end()); // has been erased

  connectToHub();
  m_entries["/test/A"].fail(nullptr);
  testAfterRibInsert("/test/A/app");
  BOOST_CHECK(m_requests.empty()); // previous propagation has not succeeded
  BOOST_CHECK(m_entries.find("/test/A") == m_entries.end()); // has been erased

  m_entries["/test/A"].succeed(nullptr);
  testAfterRibInsert("/test/A/app");
  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "unregister", "/test/A"), CheckRequestResult::OK);
  BOOST_CHECK(m_entries.find("/test/A") == m_entries.end()); // has been erased
}

BOOST_AUTO_TEST_CASE(AfterHubConnectDisconnect)
{
  BOOST_REQUIRE(addIdentity("/test/A"));
  BOOST_REQUIRE(addIdentity("/test/B"));
  BOOST_REQUIRE(addIdentity("/test/C"));
  BOOST_REQUIRE(insertEntryToRib("/test/A/app"));
  BOOST_REQUIRE(insertEntryToRib("/test/B/app"));

  // recorder the prefixes that will be propagated in order
  std::vector<Name> propagatedPrefixes;

  BOOST_CHECK(m_requests.empty()); // no request because there is no connectivity to the hub now
  BOOST_REQUIRE_EQUAL(m_entries.size(), 2); // valid entries will be kept

  connectToHub(); // 2 cached entries will be processed
  for (auto&& entry : m_entries) {
    propagatedPrefixes.push_back(entry.first);
  }

  BOOST_REQUIRE(insertEntryToRib("/test/C/app")); // will be processed directly
  propagatedPrefixes.push_back("/test/C");
  BOOST_REQUIRE_EQUAL(m_entries.size(), 3);

  BOOST_REQUIRE(m_entries["/test/A"].isPropagating());
  BOOST_REQUIRE(m_entries["/test/B"].isPropagating());
  BOOST_REQUIRE(m_entries["/test/C"].isPropagating());

  disconnectFromHub(); // all 3 entries are initialized
  BOOST_CHECK(m_entries["/test/A"].isNew());
  BOOST_CHECK(m_entries["/test/B"].isNew());
  BOOST_CHECK(m_entries["/test/C"].isNew());

  connectToHub(); // all 3 entries will be processed
  for (auto&& entry : m_entries) {
    propagatedPrefixes.push_back(entry.first);
  }

  BOOST_REQUIRE_EQUAL(m_requests.size(), 6);
  BOOST_REQUIRE_EQUAL(propagatedPrefixes.size(), 6);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", propagatedPrefixes[0]), CheckRequestResult::OK);
  BOOST_CHECK_EQUAL(checkRequest(1, "register", propagatedPrefixes[1]), CheckRequestResult::OK);
  BOOST_CHECK_EQUAL(checkRequest(2, "register", propagatedPrefixes[2]), CheckRequestResult::OK);
  BOOST_CHECK_EQUAL(checkRequest(3, "register", propagatedPrefixes[3]), CheckRequestResult::OK);
  BOOST_CHECK_EQUAL(checkRequest(4, "register", propagatedPrefixes[4]), CheckRequestResult::OK);
  BOOST_CHECK_EQUAL(checkRequest(5, "register", propagatedPrefixes[5]), CheckRequestResult::OK);
}

BOOST_AUTO_TEST_CASE(AfterPropagateSucceed)
{
  bool wasRefreshEventTriggered = false;
  auto testAfterPropagateSucceed = [&] (const Name& ribEntryPrefix) {
    m_requests.clear();
    wasRefreshEventTriggered = false;

    auto propagateParameters = m_propagator.getPrefixPropagationParameters(ribEntryPrefix);
    m_propagator.afterPropagateSucceed(propagateParameters.parameters, propagateParameters.options,
                                       [&]{ wasRefreshEventTriggered = true; });
    advanceClocks(time::milliseconds(1));
  };

  BOOST_REQUIRE(addIdentity("/test/A"));
  m_propagator.m_refreshInterval = time::seconds(0); // event will be executed at once
  m_entries["/test/A"].startPropagation(); // set to be in PROPAGATING state

  testAfterPropagateSucceed("/test/A/app"); // will trigger refresh event
  BOOST_CHECK(wasRefreshEventTriggered);
  BOOST_CHECK(m_requests.empty());

  m_entries.erase(m_entries.find("/test/A")); // set to be in RELEASED state
  testAfterPropagateSucceed("/test/A/app"); // will call startRevocation
  BOOST_CHECK(!wasRefreshEventTriggered);
  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "unregister", "/test/A"), CheckRequestResult::OK);
}

BOOST_AUTO_TEST_CASE(AfterPropagateFail)
{
  bool wasRetryEventTriggered = false;
  auto testAfterPropagateFail = [&] (const Name& ribEntryPrefix) {
    m_requests.clear();
    wasRetryEventTriggered = false;

    auto propagateParameters = m_propagator.getPrefixPropagationParameters(ribEntryPrefix);
    m_propagator.afterPropagateFail(ndn::nfd::ControlResponse(400, "test"),
                                    propagateParameters.parameters, propagateParameters.options,
                                    time::seconds(0), [&] { wasRetryEventTriggered = true; });
    advanceClocks(time::milliseconds(1));
  };

  BOOST_REQUIRE(addIdentity("/test/A"));
  m_entries["/test/A"].startPropagation(); // set to be in PROPAGATING state

  testAfterPropagateFail("/test/A/app"); // will trigger retry event
  BOOST_CHECK(wasRetryEventTriggered);
  BOOST_CHECK(m_requests.empty());

  m_entries.erase(m_entries.find("/test/A")); // set to be in RELEASED state
  testAfterPropagateFail("/test/A/app"); // will do nothing
  BOOST_CHECK(!wasRetryEventTriggered);
  BOOST_CHECK(m_requests.empty());
}

BOOST_AUTO_TEST_CASE(AfterRevokeSucceed)
{
  auto testAfterRevokeSucceed = [&] (const Name& ribEntryPrefix) {
    m_requests.clear();
    auto propagateParameters = m_propagator.getPrefixPropagationParameters(ribEntryPrefix);
    m_propagator.afterRevokeSucceed(propagateParameters.parameters,
                                    propagateParameters.options,
                                    time::seconds(0));
    advanceClocks(time::milliseconds(1));
  };

  BOOST_REQUIRE(addIdentity("/test/A"));

  testAfterRevokeSucceed("/test/A/app"); // in RELEASED state
  BOOST_CHECK(m_requests.empty());

  m_entries["/test/A"].fail(nullptr); // in PROPAGATE_FAIL state
  testAfterRevokeSucceed("/test/A/app");
  BOOST_CHECK(m_requests.empty());

  m_entries["/test/A"].succeed(nullptr); // in PROPAGATED state
  testAfterRevokeSucceed("/test/A/app");
  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/A"), CheckRequestResult::OK);

  m_entries["/test/A"].startPropagation(); // in PROPAGATING state
  testAfterRevokeSucceed("/test/A/app");
  BOOST_REQUIRE_EQUAL(m_requests.size(), 1);
  BOOST_CHECK_EQUAL(checkRequest(0, "register", "/test/A"), CheckRequestResult::OK);
}

BOOST_AUTO_TEST_SUITE_END() // PropagatedEntryStateChanges

BOOST_AUTO_TEST_SUITE_END() // TestAutoPrefixPropagator

} // namespace tests
} // namespace rib
} // namespace nfd
