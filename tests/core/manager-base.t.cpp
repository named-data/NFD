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

#include "core/manager-base.hpp"
#include "tests/manager-common-fixture.hpp"

#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/pib/identity.hpp>
#include <ndn-cxx/security/pib/key.hpp>
#include <ndn-cxx/security/pib/pib.hpp>

namespace nfd {
namespace tests {

class TestCommandVoidParameters : public ndn::nfd::ControlCommand
{
public:
  TestCommandVoidParameters()
    : ndn::nfd::ControlCommand("test-module", "test-void-parameters")
  {
  }
};

class TestCommandRequireName : public ndn::nfd::ControlCommand
{
public:
  TestCommandRequireName()
    : ndn::nfd::ControlCommand("test-module", "test-require-name")
  {
    m_requestValidator.required(ndn::nfd::CONTROL_PARAMETER_NAME);
  }
};

class ManagerTester : public ManagerBase
{
public:
  using ManagerBase::ManagerBase;

  ndn::mgmt::Authorization
  makeAuthorization(const std::string& verb) override
  {
    return [] (const Name& prefix, const Interest& interest,
               const ndn::mgmt::ControlParameters* params,
               ndn::mgmt::AcceptContinuation accept,
               ndn::mgmt::RejectContinuation reject) {
      accept("requester");
    };
  }
};

class ManagerBaseFixture : public ManagerCommonFixture
{
public:
  ManagerBaseFixture()
    : m_manager(m_dispatcher, "test-module")
  {
  }

protected:
  ManagerTester m_manager;
};

BOOST_FIXTURE_TEST_SUITE(TestManagerBase, ManagerBaseFixture)

BOOST_AUTO_TEST_CASE(RegisterCommandHandler)
{
  bool wasCommandHandlerCalled = false;
  auto handler = bind([&] { wasCommandHandlerCalled = true; });

  m_manager.registerCommandHandler<TestCommandVoidParameters>("test-void", handler);
  m_manager.registerCommandHandler<TestCommandRequireName>("test-require-name", handler);
  setTopPrefix("/localhost/nfd");

  auto testRegisterCommandHandler = [&wasCommandHandlerCalled, this] (const Name& commandName) {
    wasCommandHandlerCalled = false;
    receiveInterest(makeControlCommandRequest(commandName, ControlParameters()));
  };

  testRegisterCommandHandler("/localhost/nfd/test-module/test-void");
  BOOST_CHECK(wasCommandHandlerCalled);

  testRegisterCommandHandler("/localhost/nfd/test-module/test-require-name");
  BOOST_CHECK(!wasCommandHandlerCalled);
}

BOOST_AUTO_TEST_CASE(RegisterStatusDataset)
{
  bool isStatusDatasetCalled = false;
  auto handler = bind([&] { isStatusDatasetCalled = true; });

  m_manager.registerStatusDatasetHandler("test-status", handler);
  setTopPrefix("/localhost/nfd");

  receiveInterest(Interest("/localhost/nfd/test-module/test-status"));
  BOOST_CHECK(isStatusDatasetCalled);
}

BOOST_AUTO_TEST_CASE(RegisterNotificationStream)
{
  auto post = m_manager.registerNotificationStream("test-notification");
  setTopPrefix("/localhost/nfd");

  const uint8_t buf[] = {0x82, 0x01, 0x02};
  post(Block(buf, sizeof(buf)));
  advanceClocks(time::milliseconds(1));

  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(m_responses[0].getName(),
                    Name("/localhost/nfd/test-module/test-notification/%FE%00"));
}

BOOST_AUTO_TEST_CASE(ExtractRequester)
{
  std::string requesterName;
  auto testAccept = [&] (const std::string& requester) { requesterName = requester; };

  m_manager.extractRequester(Interest("/test/interest/unsigned"), testAccept);
  BOOST_CHECK(requesterName.empty());

  requesterName = "";
  m_manager.extractRequester(makeControlCommandRequest("/test/interest/signed", ControlParameters()), testAccept);
  auto keyLocator = m_keyChain.getPib().getIdentity(DEFAULT_COMMAND_SIGNER_IDENTITY).getDefaultKey().getName();
  BOOST_CHECK_EQUAL(requesterName, keyLocator.toUri());
}

BOOST_AUTO_TEST_CASE(ValidateParameters)
{
  ControlParameters params;
  TestCommandVoidParameters commandVoidParams;
  TestCommandRequireName commandRequireName;

  BOOST_CHECK_EQUAL(ManagerBase::validateParameters(commandVoidParams, params), true); // succeeds
  BOOST_CHECK_EQUAL(ManagerBase::validateParameters(commandRequireName, params), false); // fails

  params.setName("test-name");
  BOOST_CHECK_EQUAL(ManagerBase::validateParameters(commandRequireName, params), true); // succeeds
}

BOOST_AUTO_TEST_CASE(MakeRelPrefix)
{
  auto generatedRelPrefix = m_manager.makeRelPrefix("test-verb");
  BOOST_CHECK_EQUAL(generatedRelPrefix, PartialName("/test-module/test-verb"));
}

BOOST_AUTO_TEST_SUITE_END() // TestManagerBase

} // namespace tests
} // namespace nfd
