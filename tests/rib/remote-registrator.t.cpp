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

#include "rib/remote-registrator.hpp"

#include "tests/limited-io.hpp"
#include "tests/identity-management-fixture.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd {
namespace rib {
namespace tests {

class RemoteRegistratorFixture : public nfd::tests::IdentityManagementFixture
                               , public nfd::tests::UnitTestTimeFixture
{
public:
  RemoteRegistratorFixture()
    : face(ndn::util::makeDummyClientFace(getGlobalIoService()))
    , controller(make_shared<ndn::nfd::Controller>(std::ref(*face), m_keyChain))
    , remoteRegistrator(make_shared<RemoteRegistrator>(std::ref(*controller),
                                                       m_keyChain,
                                                       rib))
    , COMMAND_PREFIX("/localhop/nfd/rib")
    , REGISTER_VERB("register")
    , UNREGISTER_VERB("unregister")
  {
    readConfig();

    remoteRegistrator->enable();

    advanceClocks(time::milliseconds(1));
    face->sentInterests.clear();
  }

  void
  readConfig(bool isSetRetry = false)
  {
    ConfigFile config;
    config.addSectionHandler("remote_register",
                             bind(&RemoteRegistrator::loadConfig, remoteRegistrator, _1));


    if (isSetRetry) {
      const std::string CONFIG_STRING =
      "remote_register\n"
      "{\n"
      "  cost 15\n"
      "  timeout 1000\n"
      "  retry 1\n"
      "  refresh_interval 5\n"
      "}";

      config.parse(CONFIG_STRING, true, "test-remote-register");
    }
    else {
      const std::string CONFIG_STRING =
      "remote_register\n"
      "{\n"
      "  cost 15\n"
      "  timeout 100000\n"
      "  retry 0\n"
      "  refresh_interval 5\n"
      "}";

      config.parse(CONFIG_STRING, true, "test-remote-register");
    }
  }

  void
  waitForTimeout()
  {
    advanceClocks(time::milliseconds(100), time::seconds(1));
  }

  void
  insertEntryWithIdentity(Name identity,
                          name::Component appName = DEFAULT_APP_NAME,
                          uint64_t faceId = 0)
  {
    BOOST_CHECK_EQUAL(addIdentity(identity), true);

    Route route;
    route.faceId = faceId;

    rib.insert(identity.append(appName), route);

    advanceClocks(time::milliseconds(1));
  }

  void
  insertEntryWithoutIdentity(Name identity,
                             name::Component appName = DEFAULT_APP_NAME,
                             uint64_t faceId = 0)
  {
    Route route;
    route.faceId = faceId;

    rib.insert(identity.append(appName), route);

    advanceClocks(time::milliseconds(1));
  }

  void
  eraseEntryWithIdentity(Name identity,
                         name::Component appName = DEFAULT_APP_NAME,
                         uint64_t faceId = 0)
  {
    BOOST_CHECK_EQUAL(addIdentity(identity), true);

    Route route;
    route.faceId = faceId;

    rib.erase(identity.append(appName), route);

    advanceClocks(time::milliseconds(1));
  }

  void
  eraseEntryWithoutIdentity(Name identity,
                            name::Component appName = DEFAULT_APP_NAME,
                            uint64_t faceId = 0)
  {
    Route route;
    route.faceId = faceId;

    rib.erase(identity.append(appName), route);

    advanceClocks(time::milliseconds(1));
  }

  void
  eraseFace(uint64_t faceId)
  {
    for (const Rib::NameAndRoute& item : rib.findRoutesWithFaceId(faceId)) {
      rib.erase(item.first, item.second);
    }

    advanceClocks(time::milliseconds(1));
  }

  void
  connectToHub()
  {
    rib.insert(Name("/localhop/nfd"), Route());

    advanceClocks(time::milliseconds(1));
  }

  void
  disconnectToHub()
  {
    rib.erase(Name("/localhop/nfd"), Route());

    advanceClocks(time::milliseconds(1));
  }

  void
  extractParameters(Interest& interest, Name::Component& verb,
                    ndn::nfd::ControlParameters& extractedParameters)
  {
    const Name& name = interest.getName();
    verb = name[COMMAND_PREFIX.size()];
    const Name::Component& parameterComponent = name[COMMAND_PREFIX.size() + 1];

    Block rawParameters = parameterComponent.blockFromValue();
    extractedParameters.wireDecode(rawParameters);
  }

public:
  Rib rib;
  shared_ptr<ndn::util::DummyClientFace> face;
  shared_ptr<ndn::nfd::Controller> controller;
  shared_ptr<RemoteRegistrator> remoteRegistrator;

  const Name COMMAND_PREFIX;
  const name::Component REGISTER_VERB;
  const name::Component UNREGISTER_VERB;

  static const name::Component DEFAULT_APP_NAME;
};

const name::Component RemoteRegistratorFixture::DEFAULT_APP_NAME("app");

BOOST_FIXTURE_TEST_SUITE(TestRemoteRegistrator, RemoteRegistratorFixture)

BOOST_FIXTURE_TEST_CASE(AutoTest, RemoteRegistratorFixture)
{
  BOOST_REQUIRE_EQUAL(1, 1);
}

BOOST_FIXTURE_TEST_CASE(RegisterWithoutConnection, RemoteRegistratorFixture)
{
  insertEntryWithIdentity("/remote/register");

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(RegisterWithoutIdentity, RemoteRegistratorFixture)
{
  connectToHub();

  insertEntryWithoutIdentity("/remote/register");

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(RegisterWithHubPrefix, RemoteRegistratorFixture)
{
  connectToHub();

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(RegisterWithLocalPrefix, RemoteRegistratorFixture)
{
  connectToHub();

  insertEntryWithIdentity("/localhost/prefix");

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(RegisterBasic, RemoteRegistratorFixture)
{
  connectToHub();

  Name identity("/remote/register");
  insertEntryWithIdentity(identity);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 1);

  Interest& request = face->sentInterests[0];

  ndn::nfd::ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, REGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), identity);
}

BOOST_FIXTURE_TEST_CASE(RegisterAdvanced, RemoteRegistratorFixture)
{
  connectToHub();

  Name identity("/remote/register");
  Name identityAddRib("/remote/register/rib");
  insertEntryWithIdentity(identityAddRib);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 1);

  Interest& request = face->sentInterests[0];

  ndn::nfd::ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, REGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), identity);
}

BOOST_FIXTURE_TEST_CASE(RegisterWithRedundantCallback, RemoteRegistratorFixture)
{
  remoteRegistrator->enable();

  connectToHub();

  Name identity("/remote/register");
  insertEntryWithIdentity(identity);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 1);

  Interest& request = face->sentInterests[0];

  ndn::nfd::ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, REGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), identity);
}

BOOST_FIXTURE_TEST_CASE(RegisterRetry, RemoteRegistratorFixture)
{
  // setRetry
  readConfig(true);

  connectToHub();

  Name identity("/remote/register");
  insertEntryWithIdentity(identity);

  waitForTimeout();

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 2);

  Interest& requestFirst  = face->sentInterests[0];
  Interest& requestSecond = face->sentInterests[1];

  ndn::nfd::ControlParameters extractedParametersFirst, extractedParametersSecond;
  Name::Component verbFirst, verbSecond;
  extractParameters(requestFirst,  verbFirst,  extractedParametersFirst);
  extractParameters(requestSecond, verbSecond, extractedParametersSecond);

  BOOST_CHECK_EQUAL(verbFirst,  REGISTER_VERB);
  BOOST_CHECK_EQUAL(verbSecond, REGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParametersFirst.getName(),  identity);
  BOOST_CHECK_EQUAL(extractedParametersSecond.getName(), identity);
}

BOOST_FIXTURE_TEST_CASE(UnregisterWithoutInsert, RemoteRegistratorFixture)
{
  connectToHub();

  eraseEntryWithIdentity("/remote/register");

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(UnregisterWithoutConnection, RemoteRegistratorFixture)
{
  connectToHub();

  disconnectToHub();

  Name indentity("/remote/register");
  remoteRegistrator->m_regEntries.insert(
            nfd::rib::RemoteRegistrator::RegisteredEntry(indentity, scheduler::EventId()));

  eraseEntryWithIdentity(indentity);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(UnregisterWithoutSuccessfullRegistration,
                        RemoteRegistratorFixture)
{
  connectToHub();

  Name identity("/remote/register");

  insertEntryWithIdentity(identity);

  eraseEntryWithIdentity(identity);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 1);

  Interest& request = face->sentInterests[0];

  ndn::nfd::ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, REGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), identity);
}

BOOST_FIXTURE_TEST_CASE(UnregisterBasic, RemoteRegistratorFixture)
{
  connectToHub();

  Name identity("/remote/register");

  insertEntryWithIdentity(identity);

  scheduler::EventId event;

  remoteRegistrator->m_regEntries.insert(
          nfd::rib::RemoteRegistrator::RegisteredEntry(identity, event));

  eraseEntryWithIdentity(identity);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 2);

  Interest& request = face->sentInterests[1];

  ndn::nfd::ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, UNREGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), identity);
}

BOOST_FIXTURE_TEST_CASE(UnregisterAdvanced, RemoteRegistratorFixture)
{
  connectToHub();

  Name identityShort("/remote/register");
  Name identityLong("/remote/register/long");

  scheduler::EventId eventShort;
  scheduler::EventId eventLong;

  insertEntryWithIdentity(identityShort, name::Component("appA"));

  remoteRegistrator->m_regEntries.insert(
          nfd::rib::RemoteRegistrator::RegisteredEntry(identityShort,
                                                       eventShort));

  insertEntryWithIdentity(identityShort, name::Component("appB"));

  insertEntryWithIdentity(identityLong);

  remoteRegistrator->m_regEntries.insert(
          nfd::rib::RemoteRegistrator::RegisteredEntry(identityLong,
                                                       eventLong));

  // two registration commands are generated for identityShort and identityLong
  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 2);

  eraseEntryWithIdentity(identityShort, name::Component("appA"));

  // no unregistration command is generated as appB also exists
  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 2);

  eraseEntryWithIdentity(identityShort, name::Component("appB"));

  // one unregistration command is generated for identityShort
  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 3);

  Interest& request = face->sentInterests[2];

  ndn::nfd::ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, UNREGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), identityShort);
}

BOOST_FIXTURE_TEST_CASE(EraseFace, RemoteRegistratorFixture)
{
  connectToHub();

  Name identity("/remote/register");
  uint64_t faceId = 517;

  insertEntryWithIdentity(identity, DEFAULT_APP_NAME, faceId);

  scheduler::EventId event;

  remoteRegistrator->m_regEntries.insert(
          nfd::rib::RemoteRegistrator::RegisteredEntry(identity, event));

  eraseFace(faceId);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 2);

  Interest& request = face->sentInterests[1];

  ndn::nfd::ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, UNREGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), identity);
}

BOOST_FIXTURE_TEST_CASE(RebuildConnection, RemoteRegistratorFixture)
{
  connectToHub();

  Name identity("/remote/register");

  insertEntryWithIdentity(identity);

  scheduler::EventId event;

  remoteRegistrator->m_regEntries.insert(
          nfd::rib::RemoteRegistrator::RegisteredEntry(identity, event));

  disconnectToHub();

  connectToHub();

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 2);

  Interest& request1 = face->sentInterests[0];
  Interest& request2 = face->sentInterests[1];

  ndn::nfd::ControlParameters extractedParameters1, extractedParameters2;
  Name::Component verb1, verb2;
  extractParameters(request1, verb1, extractedParameters1);
  extractParameters(request2, verb2, extractedParameters2);

  BOOST_CHECK_EQUAL(verb1, REGISTER_VERB);
  BOOST_CHECK_EQUAL(verb2, REGISTER_VERB);
  BOOST_CHECK_EQUAL(extractedParameters1.getName(),
                    extractedParameters2.getName());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace rib
} // namespace nfd
