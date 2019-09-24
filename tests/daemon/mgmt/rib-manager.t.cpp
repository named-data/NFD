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

#include "mgmt/rib-manager.hpp"

#include "manager-common-fixture.hpp"
#include "tests/daemon/rib/fib-updates-common.hpp"

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/mgmt/nfd/rib-entry.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

#include <boost/property_tree/info_parser.hpp>

namespace nfd {
namespace tests {

struct ConfigurationStatus
{
  bool isLocalhostConfigured;
  bool isLocalhopConfigured;
};

static ConfigSection
getValidatorConfigSection()
{
  ConfigSection section;
  section.put("trust-anchor.type", "any");
  return section;
}

static ConfigSection
makeSection(const std::string& config)
{
  std::istringstream inputStream(config);
  ConfigSection section;
  boost::property_tree::read_info(inputStream, section);
  return section;
}

static ConfigSection
getLocalhopValidatorConfigSection()
{
  std::string config = R"CONF(
    rule
    {
      id rule-id1
      for interest
      filter
      {
        type name
        name /localhop/nfd/rib
        relation is-prefix-of
      }
      checker
      {
        type customized
        sig-type ecdsa-sha256
        key-locator
        {
          type name
          name /TrustAnchor-identity/Derived-identity
          relation is-prefix-of
        }
      }
    }
    rule
    {
      id rule-id2
      for data
      filter
      {
        type name
        name /TrustAnchor-identity/Derived-identity/KEY
        relation is-prefix-of
      }
      checker
      {
        type customized
        sig-type ecdsa-sha256
        key-locator
        {
          type name
          name /TrustAnchor-identity
          relation is-prefix-of
        }
      }
    }
    trust-anchor
    {
      type file
      file-name signer.ndncert
    }
  )CONF";
  return makeSection(config);
}

class RibManagerFixture : public ManagerCommonFixture
{
public:
  RibManagerFixture(const ConfigurationStatus& status, bool shouldClearRib)
    : m_status(status)
    , m_anchorId("/TrustAnchor-identity")
    , m_derivedId("/TrustAnchor-identity/Derived-identity")
    , m_nfdController(m_face, m_keyChain)
    , m_fibUpdater(m_rib, m_nfdController)
    , m_manager(m_rib, m_face, m_keyChain, m_nfdController, m_dispatcher)
  {
    addIdentity(m_anchorId);
    addIdentity(m_derivedId);

    m_derivedCert = m_keyChain.getPib().getIdentity(m_derivedId).getDefaultKey().getDefaultCertificate();
    ndn::SignatureInfo signatureInfo;
    signatureInfo.setValidityPeriod(m_derivedCert.getValidityPeriod());
    ndn::security::SigningInfo signingInfo(ndn::security::SigningInfo::SIGNER_TYPE_ID,
                                           m_anchorId, signatureInfo);
    m_keyChain.sign(m_derivedCert, signingInfo);
    saveIdentityCertificate(m_anchorId, "signer.ndncert", true);

    if (m_status.isLocalhostConfigured) {
      m_manager.applyLocalhostConfig(getValidatorConfigSection(), "test");
    }
    if (m_status.isLocalhopConfigured) {
      m_manager.enableLocalhop(getLocalhopValidatorConfigSection(), "test");
    }
    else {
      m_manager.disableLocalhop();
    }

    registerWithNfd();

    if (shouldClearRib) {
      clearRib();
    }

    m_face.onSendInterest.connect([this] (const Interest& interest) {
      if (interest.getName().isPrefixOf(m_derivedCert.getName())) {
        if (m_status.isLocalhopConfigured && interest.template getTag<lp::NextHopFaceIdTag>() != nullptr) {
          m_face.put(m_derivedCert);
        }
      }
    });
  }

private:
  void
  registerWithNfd()
  {
    m_manager.registerWithNfd();
    advanceClocks(1_ms);

    auto replyFibAddCommand = [this] (const Interest& interest) {
      ControlParameters params(interest.getName().get(-5).blockFromValue());
      BOOST_CHECK(params.getName() == "/localhost/nfd/rib" || params.getName() == "/localhop/nfd/rib");
      params.setFaceId(1).setCost(0);
      ControlResponse resp;

      resp.setCode(200).setBody(params.wireEncode());
      shared_ptr<Data> data = make_shared<Data>(interest.getName());
      data->setContent(resp.wireEncode());

      m_keyChain.sign(*data, ndn::security::SigningInfo(ndn::security::SigningInfo::SIGNER_TYPE_SHA256));

      m_face.getIoService().post([this, data] { m_face.receive(*data); });
    };

    Name commandPrefix("/localhost/nfd/fib/add-nexthop");
    for (const auto& command : m_face.sentInterests) {
      if (commandPrefix.isPrefixOf(command.getName())) {
        replyFibAddCommand(command);
        advanceClocks(1_ms);
      }
    }

    // clear commands and responses
    m_responses.clear();
    m_face.sentInterests.clear();
  }

  void
  clearRib()
  {
    while (!m_rib.empty()) {
      m_rib.erase(m_rib.begin()->first, *m_rib.begin()->second->begin());
    }
  }

public:
  static ControlParameters
  makeRegisterParameters(const Name& name, uint64_t faceId = 0,
                         time::milliseconds expiry = time::milliseconds::max())
  {
    return ControlParameters()
      .setName(name)
      .setFaceId(faceId)
      .setOrigin(ndn::nfd::ROUTE_ORIGIN_NLSR)
      .setCost(10)
      .setFlags(0)
      .setExpirationPeriod(expiry);
  }

  static ControlParameters
  makeUnregisterParameters(const Name& name, uint64_t faceId = 0)
  {
    return ControlParameters()
      .setName(name)
      .setFaceId(faceId)
      .setOrigin(ndn::nfd::ROUTE_ORIGIN_NLSR);
  }

protected:
  ConfigurationStatus m_status;
  Name m_anchorId;
  Name m_derivedId;
  ndn::security::v2::Certificate m_derivedCert;

  ndn::nfd::Controller m_nfdController;
  rib::Rib m_rib;
  rib::tests::MockFibUpdater m_fibUpdater;
  RibManager m_manager;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_AUTO_TEST_SUITE(TestRibManager)

class AddTopPrefixFixture : public RibManagerFixture
{
public:
  AddTopPrefixFixture()
    : RibManagerFixture({true, true}, false)
  {
  }
};

BOOST_FIXTURE_TEST_CASE(AddTopPrefix, AddTopPrefixFixture)
{
  BOOST_CHECK_EQUAL(m_rib.size(), 2);

  std::vector<Name> ribEntryNames;
  for (auto&& entry : m_rib) {
    ribEntryNames.push_back(entry.first);
  }
  BOOST_CHECK_EQUAL(ribEntryNames[0], "/localhop/nfd");
  BOOST_CHECK_EQUAL(ribEntryNames[1], "/localhost/nfd");
}

class UnauthorizedRibManagerFixture : public RibManagerFixture
{
public:
  UnauthorizedRibManagerFixture()
    : RibManagerFixture({false, false}, true)
  {
  }
};

class LocalhostAuthorizedRibManagerFixture : public RibManagerFixture
{
public:
  LocalhostAuthorizedRibManagerFixture()
    : RibManagerFixture({true, false}, true)
  {
  }
};

class LocalhopAuthorizedRibManagerFixture : public RibManagerFixture
{
public:
  LocalhopAuthorizedRibManagerFixture()
    : RibManagerFixture({false, true}, true)
  {
  }
};

class AuthorizedRibManagerFixture : public RibManagerFixture
{
public:
  AuthorizedRibManagerFixture()
    : RibManagerFixture({true, true}, true)
  {
  }
};

using AllFixtures = boost::mpl::vector<
  UnauthorizedRibManagerFixture,
  LocalhostAuthorizedRibManagerFixture,
  LocalhopAuthorizedRibManagerFixture,
  AuthorizedRibManagerFixture
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(CommandAuthorization, T, AllFixtures, T)
{
  auto parameters  = this->makeRegisterParameters("/test-authorization", 9527);
  auto commandHost = this->makeControlCommandRequest("/localhost/nfd/rib/register", parameters);
  auto commandHop  = this->makeControlCommandRequest("/localhop/nfd/rib/register", parameters,
                                                     this->m_derivedId);
  if (this->m_status.isLocalhopConfigured) {
    commandHop.setTag(make_shared<lp::IncomingFaceIdTag>(123));
  }
  auto successResp = this->makeResponse(200, "Success", parameters);
  auto failureResp = ControlResponse(403, "authorization rejected");

  BOOST_CHECK_EQUAL(this->m_responses.size(), 0);
  this->receiveInterest(commandHost);
  this->receiveInterest(commandHop);

  auto nExpectedResponses = this->m_status.isLocalhopConfigured ? 2 : 1;
  auto expectedLocalhostResponse = this->m_status.isLocalhostConfigured ? successResp : failureResp;
  auto expectedLocalhopResponse = this->m_status.isLocalhopConfigured ? successResp : failureResp;

  BOOST_REQUIRE_EQUAL(this->m_responses.size(), nExpectedResponses);
  BOOST_CHECK_EQUAL(this->checkResponse(0, commandHost.getName(), expectedLocalhostResponse),
                    ManagerCommonFixture::CheckResponseResult::OK);
  if (nExpectedResponses == 2) {
    BOOST_CHECK_EQUAL(this->checkResponse(1, commandHop.getName(), expectedLocalhopResponse),
                      ManagerCommonFixture::CheckResponseResult::OK);
  }
}

BOOST_FIXTURE_TEST_SUITE(RegisterUnregister, LocalhostAuthorizedRibManagerFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  auto paramsRegister    = makeRegisterParameters("/test-register-unregister", 9527);
  auto paramsUnregister  = makeUnregisterParameters("/test-register-unregister", 9527);

  auto commandRegister = makeControlCommandRequest("/localhost/nfd/rib/register", paramsRegister);
  commandRegister.setTag(make_shared<lp::IncomingFaceIdTag>(1234));
  auto commandUnregister = makeControlCommandRequest("/localhost/nfd/rib/unregister", paramsUnregister);
  commandUnregister.setTag(make_shared<lp::IncomingFaceIdTag>(1234));

  receiveInterest(commandRegister);
  receiveInterest(commandUnregister);

  BOOST_REQUIRE_EQUAL(m_responses.size(), 2);
  BOOST_CHECK_EQUAL(checkResponse(0, commandRegister.getName(), makeResponse(200, "Success", paramsRegister)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkResponse(1, commandUnregister.getName(), makeResponse(200, "Success", paramsUnregister)),
                    CheckResponseResult::OK);

  BOOST_REQUIRE_EQUAL(m_fibUpdater.updates.size(), 2);
  BOOST_CHECK_EQUAL(m_fibUpdater.updates.front(),
                    rib::FibUpdate::createAddUpdate("/test-register-unregister", 9527, 10));
  BOOST_CHECK_EQUAL(m_fibUpdater.updates.back(),
                    rib::FibUpdate::createRemoveUpdate("/test-register-unregister", 9527));
}

BOOST_AUTO_TEST_CASE(SelfOperation)
{
  auto paramsRegister = makeRegisterParameters("/test-self-register-unregister");
  auto paramsUnregister = makeUnregisterParameters("/test-self-register-unregister");
  BOOST_CHECK_EQUAL(paramsRegister.getFaceId(), 0);
  BOOST_CHECK_EQUAL(paramsUnregister.getFaceId(), 0);

  const uint64_t inFaceId = 9527;
  auto commandRegister = makeControlCommandRequest("/localhost/nfd/rib/register", paramsRegister);
  commandRegister.setTag(make_shared<lp::IncomingFaceIdTag>(inFaceId));
  auto commandUnregister = makeControlCommandRequest("/localhost/nfd/rib/unregister", paramsUnregister);
  commandUnregister.setTag(make_shared<lp::IncomingFaceIdTag>(inFaceId));

  receiveInterest(commandRegister);
  receiveInterest(commandUnregister);

  paramsRegister.setFaceId(inFaceId);
  paramsUnregister.setFaceId(inFaceId);

  BOOST_REQUIRE_EQUAL(m_responses.size(), 2);
  BOOST_CHECK_EQUAL(checkResponse(0, commandRegister.getName(), makeResponse(200, "Success", paramsRegister)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkResponse(1, commandUnregister.getName(), makeResponse(200, "Success", paramsUnregister)),
                    CheckResponseResult::OK);

  BOOST_REQUIRE_EQUAL(m_fibUpdater.updates.size(), 2);
  BOOST_CHECK_EQUAL(m_fibUpdater.updates.front(),
                    rib::FibUpdate::createAddUpdate("/test-self-register-unregister", 9527, 10));
  BOOST_CHECK_EQUAL(m_fibUpdater.updates.back(),
                    rib::FibUpdate::createRemoveUpdate("/test-self-register-unregister", 9527));
}

BOOST_AUTO_TEST_CASE(Expiration)
{
  auto paramsRegister = makeRegisterParameters("/test-expiry", 9527, 50_ms);
  auto paramsUnregister = makeRegisterParameters("/test-expiry", 9527);
  receiveInterest(makeControlCommandRequest("/localhost/nfd/rib/register", paramsRegister));

  advanceClocks(55_ms);
  BOOST_REQUIRE_EQUAL(m_fibUpdater.updates.size(), 2); // the registered route has expired
  BOOST_CHECK_EQUAL(m_fibUpdater.updates.front(),
                    rib::FibUpdate::createAddUpdate("/test-expiry", 9527, 10));
  BOOST_CHECK_EQUAL(m_fibUpdater.updates.back(),
                    rib::FibUpdate::createRemoveUpdate("/test-expiry", 9527));

  m_fibUpdater.updates.clear();
  paramsRegister.setExpirationPeriod(100_ms);
  receiveInterest(makeControlCommandRequest("/localhost/nfd/rib/register", paramsRegister));

  advanceClocks(55_ms);
  BOOST_REQUIRE_EQUAL(m_fibUpdater.updates.size(), 1); // the registered route is still active
  BOOST_CHECK_EQUAL(m_fibUpdater.updates.front(),
                    rib::FibUpdate::createAddUpdate("/test-expiry", 9527, 10));
}

BOOST_AUTO_TEST_CASE(NameTooLong)
{
  Name prefix;
  while (prefix.size() <= Fib::getMaxDepth()) {
    prefix.append("A");
  }
  auto params = makeRegisterParameters(prefix, 2899);
  auto command = makeControlCommandRequest("/localhost/nfd/rib/register", params);
  receiveInterest(command);

  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, command.getName(),
                                  ControlResponse(414, "Route prefix cannot exceed " +
                                                  to_string(Fib::getMaxDepth()) + " components")),
                    CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(m_fibUpdater.updates.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // RegisterUnregister

BOOST_FIXTURE_TEST_CASE(RibDataset, UnauthorizedRibManagerFixture)
{
  uint64_t faceId = 0;
  auto generateRoute = [&faceId] () -> rib::Route {
    rib::Route route;
    route.faceId = ++faceId;
    route.cost = route.faceId * 10;
    route.expires = nullopt;
    return route;
  };

  const size_t nEntries = 108;
  std::set<Name> actualPrefixes;
  for (size_t i = 0; i < nEntries; ++i) {
    Name prefix = Name("/test-dataset").appendNumber(i);
    actualPrefixes.insert(prefix);
    m_rib.insert(prefix, generateRoute());
    if (i & 0x1) {
      m_rib.insert(prefix, generateRoute());
      m_rib.insert(prefix, generateRoute());
    }
  }

  receiveInterest(*makeInterest("/localhost/nfd/rib/list", true));

  Block content = concatenateResponses();
  content.parse();
  BOOST_REQUIRE_EQUAL(content.elements().size(), nEntries);

  std::vector<ndn::nfd::RibEntry> receivedRecords, expectedRecords;
  for (size_t idx = 0; idx < nEntries; ++idx) {
    ndn::nfd::RibEntry decodedEntry(content.elements()[idx]);
    receivedRecords.push_back(decodedEntry);
    actualPrefixes.erase(decodedEntry.getName());

    auto matchedEntryIt = m_rib.find(decodedEntry.getName());
    BOOST_REQUIRE(matchedEntryIt != m_rib.end());

    auto matchedEntry = matchedEntryIt->second;
    BOOST_REQUIRE(matchedEntry != nullptr);

    expectedRecords.emplace_back();
    expectedRecords.back().setName(matchedEntry->getName());
    for (const auto& route : matchedEntry->getRoutes()) {
      expectedRecords.back().addRoute(ndn::nfd::Route()
                                      .setFaceId(route.faceId)
                                      .setOrigin(route.origin)
                                      .setCost(route.cost)
                                      .setFlags(route.flags));
    }
  }

  BOOST_CHECK_EQUAL(actualPrefixes.size(), 0);
  BOOST_CHECK_EQUAL_COLLECTIONS(receivedRecords.begin(), receivedRecords.end(),
                                expectedRecords.begin(), expectedRecords.end());
}

BOOST_FIXTURE_TEST_SUITE(FaceMonitor, LocalhostAuthorizedRibManagerFixture)

BOOST_AUTO_TEST_CASE(FetchActiveFacesEvent)
{
  BOOST_CHECK_EQUAL(m_fibUpdater.updates.size(), 0);

  advanceClocks(301_s); // RibManager::ACTIVE_FACE_FETCH_INTERVAL = 300s
  BOOST_REQUIRE_EQUAL(m_face.sentInterests.size(), 2);
  BOOST_CHECK_EQUAL(m_face.sentInterests[0].getName(), "/localhost/nfd/faces/events");
  BOOST_CHECK_EQUAL(m_face.sentInterests[1].getName(), "/localhost/nfd/faces/list");
}

BOOST_AUTO_TEST_CASE(RemoveInvalidFaces)
{
  auto parameters1 = makeRegisterParameters("/test-remove-invalid-faces-1");
  auto parameters2 = makeRegisterParameters("/test-remove-invalid-faces-2");
  receiveInterest(makeControlCommandRequest("/localhost/nfd/rib/register", parameters1.setFaceId(1)));
  receiveInterest(makeControlCommandRequest("/localhost/nfd/rib/register", parameters1.setFaceId(2)));
  receiveInterest(makeControlCommandRequest("/localhost/nfd/rib/register", parameters2.setFaceId(2)));
  BOOST_REQUIRE_EQUAL(m_rib.size(), 3);

  ndn::nfd::FaceStatus status;
  status.setFaceId(1);
  std::vector<ndn::nfd::FaceStatus> activeFaces;
  activeFaces.push_back(status);

  m_manager.removeInvalidFaces(activeFaces);
  advanceClocks(100_ms);
  BOOST_REQUIRE_EQUAL(m_rib.size(), 1);

  auto it1 = m_rib.find("/test-remove-invalid-faces-1");
  auto it2 = m_rib.find("/test-remove-invalid-faces-2");
  BOOST_CHECK(it2 == m_rib.end());
  BOOST_REQUIRE(it1 != m_rib.end());
  BOOST_CHECK(it1->second->hasFaceId(1));
  BOOST_CHECK(!it1->second->hasFaceId(2));
}

BOOST_AUTO_TEST_CASE(OnNotification)
{
  auto parameters1 = makeRegisterParameters("/test-face-event-notification-1", 1);
  auto parameters2 = makeRegisterParameters("/test-face-event-notification-2", 1);
  receiveInterest(makeControlCommandRequest("/localhost/nfd/rib/register", parameters1));
  receiveInterest(makeControlCommandRequest("/localhost/nfd/rib/register", parameters2));
  BOOST_REQUIRE_EQUAL(m_rib.size(), 2);

  auto makeNotification = [] (ndn::nfd::FaceEventKind kind, uint64_t faceId) {
    return ndn::nfd::FaceEventNotification().setKind(kind).setFaceId(faceId);
  };

  m_manager.onNotification(makeNotification(ndn::nfd::FaceEventKind::FACE_EVENT_DESTROYED, 1));
  advanceClocks(100_ms);
  BOOST_CHECK_EQUAL(m_rib.size(), 0);

  m_manager.onNotification(makeNotification(ndn::nfd::FaceEventKind::FACE_EVENT_CREATED, 2));
  advanceClocks(100_ms);
  BOOST_CHECK_EQUAL(m_rib.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // FaceMonitor

BOOST_AUTO_TEST_SUITE_END() // TestRibManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
