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

#include "rib/rib-manager.hpp"
#include <ndn-cxx/management/nfd-face-status.hpp>
#include "rib/rib-status-publisher-common.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd {
namespace rib {
namespace tests {

class RibManagerFixture : public nfd::tests::BaseFixture
{
public:
  RibManagerFixture()
    : COMMAND_PREFIX("/localhost/nfd/rib")
    , ADD_NEXTHOP_VERB("add-nexthop")
    , REMOVE_NEXTHOP_VERB("remove-nexthop")
    , REGISTER_COMMAND("/localhost/nfd/rib/register")
    , UNREGISTER_COMMAND("/localhost/nfd/rib/unregister")
  {
    face = ndn::util::makeDummyClientFace();

    manager = make_shared<RibManager>(*face, keyChain);
    manager->registerWithNfd();

    face->processEvents(time::milliseconds(1));
    face->sentInterests.clear();
  }

  ~RibManagerFixture()
  {
    manager.reset();
    face.reset();
  }

  void
  extractParameters(Interest& interest, Name::Component& verb,
                    ControlParameters& extractedParameters)
  {
    const Name& name = interest.getName();
    verb = name[COMMAND_PREFIX.size()];
    const Name::Component& parameterComponent = name[COMMAND_PREFIX.size() + 1];

    Block rawParameters = parameterComponent.blockFromValue();
    extractedParameters.wireDecode(rawParameters);
  }

  void
  receiveCommandInterest(const Name& name, ControlParameters& parameters)
  {
    Name commandName = name;
    commandName.append(parameters.wireEncode());

    Interest commandInterest(commandName);

    manager->m_managedRib.m_onSendBatchFromQueue = bind(&RibManagerFixture::onSendBatchFromQueue,
                                                        this, _1, parameters);

    face->receive(commandInterest);
    face->processEvents(time::milliseconds(1));
  }

  void
  onSendBatchFromQueue(const RibUpdateBatch& batch, const ControlParameters parameters)
  {
    BOOST_REQUIRE(batch.begin() != batch.end());
    RibUpdate update = *(batch.begin());

    Rib::UpdateSuccessCallback managerCallback =
      bind(&RibManager::onRibUpdateSuccess, manager, update);

    Rib& rib = manager->m_managedRib;

    // Simulate a successful response from NFD
    FibUpdater& updater = manager->m_fibUpdater;
    rib.onFibUpdateSuccess(batch, updater.m_inheritedRoutes, managerCallback);
  }


public:
  shared_ptr<RibManager> manager;
  shared_ptr<ndn::util::DummyClientFace> face;
  ndn::KeyChain keyChain;

  const Name COMMAND_PREFIX;
  const Name::Component ADD_NEXTHOP_VERB;
  const Name::Component REMOVE_NEXTHOP_VERB;

  const Name REGISTER_COMMAND;
  const Name UNREGISTER_COMMAND;
};

class AuthorizedRibManager : public RibManagerFixture
{
public:
  AuthorizedRibManager()
  {
    ConfigFile config;
    manager->setConfigFile(config);

    const std::string CONFIG_STRING =
    "rib\n"
    "{\n"
    "  localhost_security\n"
    "  {\n"
    "    trust-anchor\n"
    "    {\n"
    "      type any\n"
    "    }\n"
    "  }"
    "}";

    config.parse(CONFIG_STRING, true, "test-rib");
  }
};

typedef RibManagerFixture UnauthorizedRibManager;

BOOST_FIXTURE_TEST_SUITE(TestRibManager, RibManagerFixture)

BOOST_FIXTURE_TEST_CASE(ShortName, AuthorizedRibManager)
{
  Name commandName("/localhost/nfd/rib");
  ndn::nfd::ControlParameters parameters;

  receiveCommandInterest(commandName, parameters);
  // TODO verify error response
}

BOOST_FIXTURE_TEST_CASE(Basic, AuthorizedRibManager)
{
  ControlParameters parameters;
  parameters
    .setName("/hello")
    .setFaceId(1)
    .setCost(10)
    .setFlags(0)
    .setOrigin(128)
    .setExpirationPeriod(ndn::time::milliseconds::max());

  receiveCommandInterest(REGISTER_COMMAND, parameters);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 1);
}

BOOST_FIXTURE_TEST_CASE(Register, AuthorizedRibManager)
{
  ControlParameters parameters;
  parameters
    .setName("/hello")
    .setFaceId(1)
    .setCost(10)
    .setFlags(0)
    .setOrigin(128)
    .setExpirationPeriod(ndn::time::milliseconds::max());

  receiveCommandInterest(REGISTER_COMMAND, parameters);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 1);

  Interest& request = face->sentInterests[0];

  ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, ADD_NEXTHOP_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), parameters.getName());
  BOOST_CHECK_EQUAL(extractedParameters.getFaceId(), parameters.getFaceId());
  BOOST_CHECK_EQUAL(extractedParameters.getCost(), parameters.getCost());
}

BOOST_FIXTURE_TEST_CASE(Unregister, AuthorizedRibManager)
{
  ControlParameters addParameters;
  addParameters
    .setName("/hello")
    .setFaceId(1)
    .setCost(10)
    .setFlags(0)
    .setOrigin(128)
    .setExpirationPeriod(ndn::time::milliseconds::max());

  receiveCommandInterest(REGISTER_COMMAND, addParameters);
  face->sentInterests.clear();

  ControlParameters removeParameters;
  removeParameters
    .setName("/hello")
    .setFaceId(1)
    .setOrigin(128);

  receiveCommandInterest(UNREGISTER_COMMAND, removeParameters);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 1);

  Interest& request = face->sentInterests[0];

  ControlParameters extractedParameters;
  Name::Component verb;
  extractParameters(request, verb, extractedParameters);

  BOOST_CHECK_EQUAL(verb, REMOVE_NEXTHOP_VERB);
  BOOST_CHECK_EQUAL(extractedParameters.getName(), removeParameters.getName());
  BOOST_CHECK_EQUAL(extractedParameters.getFaceId(), removeParameters.getFaceId());
}


BOOST_FIXTURE_TEST_CASE(UnauthorizedCommand, UnauthorizedRibManager)
{
  ControlParameters parameters;
  parameters
    .setName("/hello")
    .setFaceId(1)
    .setCost(10)
    .setFlags(0)
    .setOrigin(128)
    .setExpirationPeriod(ndn::time::milliseconds::max());

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 0);

  receiveCommandInterest(REGISTER_COMMAND, parameters);

  BOOST_REQUIRE_EQUAL(face->sentInterests.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(RibStatusRequest, AuthorizedRibManager)
{
  Name prefix("/");

  Route route;
  route.faceId = 1;
  route.origin = 128;
  route.cost = 32;
  route.flags = ndn::nfd::ROUTE_FLAG_CAPTURE;

  manager->m_managedRib.insert(prefix, route);

  face->receive(Interest("/localhost/nfd/rib/list"));
  face->processEvents(time::milliseconds(1));

  BOOST_REQUIRE_EQUAL(face->sentDatas.size(), 1);
  RibStatusPublisherFixture::decodeRibEntryBlock(face->sentDatas[0], prefix, route);
}

BOOST_FIXTURE_TEST_CASE(CancelExpirationEvent, AuthorizedRibManager)
{
  // Register route
  ControlParameters addParameters;
  addParameters
    .setName("/expire")
    .setFaceId(1)
    .setCost(10)
    .setFlags(0)
    .setOrigin(128)
    .setExpirationPeriod(ndn::time::milliseconds(500));

  receiveCommandInterest(REGISTER_COMMAND, addParameters);
  face->sentInterests.clear();

  // Unregister route
  ControlParameters removeParameters;
  removeParameters
    .setName("/expire")
    .setFaceId(1)
    .setOrigin(128);

  receiveCommandInterest(UNREGISTER_COMMAND, removeParameters);

  // Reregister route
  addParameters.setExpirationPeriod(ndn::time::milliseconds::max());
  receiveCommandInterest(REGISTER_COMMAND, addParameters);

  nfd::tests::LimitedIo limitedIo;
  limitedIo.run(nfd::tests::LimitedIo::UNLIMITED_OPS, time::seconds(1));

  BOOST_REQUIRE_EQUAL(manager->m_managedRib.size(), 1);
}

BOOST_FIXTURE_TEST_CASE(RemoveInvalidFaces, AuthorizedRibManager)
{
  // Register valid face
  ControlParameters validParameters;
  validParameters
    .setName("/test")
    .setFaceId(1);

  receiveCommandInterest(REGISTER_COMMAND, validParameters);

  // Register invalid face
  ControlParameters invalidParameters;
  invalidParameters
    .setName("/test")
    .setFaceId(2);

  receiveCommandInterest(REGISTER_COMMAND, invalidParameters);

  BOOST_REQUIRE_EQUAL(manager->m_managedRib.size(), 2);

  // Receive status with only faceId: 1
  ndn::nfd::FaceStatus status;
  status.setFaceId(1);

  shared_ptr<Data> data = nfd::tests::makeData("/localhost/nfd/faces/list");
  data->setContent(status.wireEncode());

  shared_ptr<ndn::OBufferStream> buffer = make_shared<ndn::OBufferStream>();
  buffer->write(reinterpret_cast<const char*>(data->getContent().value()),
                data->getContent().value_size());

  manager->removeInvalidFaces(buffer);

  // Run scheduler
  nfd::tests::LimitedIo limitedIo;
  limitedIo.run(nfd::tests::LimitedIo::UNLIMITED_OPS, time::seconds(1));

  BOOST_REQUIRE_EQUAL(manager->m_managedRib.size(), 1);

  Rib::const_iterator it = manager->m_managedRib.find("/test");
  BOOST_REQUIRE(it != manager->m_managedRib.end());

  shared_ptr<RibEntry> entry = it->second;
  BOOST_CHECK_EQUAL(entry->hasFaceId(1), true);
  BOOST_CHECK_EQUAL(entry->hasFaceId(2), false);
}

BOOST_FIXTURE_TEST_CASE(LocalHopInherit, AuthorizedRibManager)
{
  using nfd::rib::RibManager;

  // Simulate NFD response
  ControlParameters result;
  result.setFaceId(261);

  manager->onNrdCommandPrefixAddNextHopSuccess(RibManager::REMOTE_COMMAND_PREFIX, result);

  // Register route that localhop prefix should inherit
  ControlParameters parameters;
  parameters
    .setName("/localhop/nfd")
    .setFaceId(262)
    .setCost(25)
    .setFlags(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  Name commandName("/localhost/nfd/rib/register");

  receiveCommandInterest(commandName, parameters);

  // REMOTE_COMMAND_PREFIX should have its original route and the inherited route
  auto it = manager->m_managedRib.find(RibManager::REMOTE_COMMAND_PREFIX);

  BOOST_REQUIRE(it != manager->m_managedRib.end());
  const RibEntry::RouteList& inheritedRoutes = (*(it->second)).getInheritedRoutes();

  BOOST_CHECK_EQUAL(inheritedRoutes.size(), 1);
  auto routeIt = inheritedRoutes.begin();

  BOOST_CHECK_EQUAL(routeIt->faceId, 262);
  BOOST_CHECK_EQUAL(routeIt->cost, 25);
}

BOOST_FIXTURE_TEST_CASE(RouteExpiration, AuthorizedRibManager)
{
  // Register route
  ControlParameters parameters;
  parameters.setName("/expire")
            .setExpirationPeriod(ndn::time::milliseconds(500));

  receiveCommandInterest(REGISTER_COMMAND, parameters);
  face->sentInterests.clear();

  BOOST_REQUIRE_EQUAL(manager->m_managedRib.size(), 1);

  // Route should expire
  nfd::tests::LimitedIo limitedIo;
  limitedIo.run(nfd::tests::LimitedIo::UNLIMITED_OPS, time::seconds(1));

  BOOST_CHECK_EQUAL(manager->m_managedRib.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(FaceDestroyEvent, AuthorizedRibManager)
{
  uint64_t faceToDestroy = 128;

  // Register valid face
  ControlParameters parameters;
  parameters.setName("/test")
            .setFaceId(faceToDestroy);

  receiveCommandInterest(REGISTER_COMMAND, parameters);
  BOOST_REQUIRE_EQUAL(manager->m_managedRib.size(), 1);

  // Don't respond with a success message from the FIB
  manager->m_managedRib.m_onSendBatchFromQueue = nullptr;

  manager->onFaceDestroyedEvent(faceToDestroy);
  BOOST_REQUIRE_EQUAL(manager->m_managedRib.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace rib
} // namespace nfd
