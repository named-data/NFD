/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#include "mgmt/manager-base.hpp"

#include "manager-common-fixture.hpp"

namespace nfd::tests {

class TestCommandVoidParameters : public ControlCommand
{
public:
  TestCommandVoidParameters()
    : ControlCommand("test-module", "test-void-parameters")
  {
  }
};

class TestCommandRequireName : public ControlCommand
{
public:
  TestCommandRequireName()
    : ControlCommand("test-module", "test-require-name")
  {
    m_requestValidator.required(ndn::nfd::CONTROL_PARAMETER_NAME);
  }
};

class DummyManager : public ManagerBase
{
public:
  explicit
  DummyManager(Dispatcher& dispatcher)
    : ManagerBase("test-module", dispatcher)
  {
  }

private:
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
protected:
  DummyManager m_manager{m_dispatcher};
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestManagerBase, ManagerBaseFixture)

BOOST_AUTO_TEST_CASE(RegisterCommandHandler)
{
  bool wasHandlerCalled = false;
  auto handler = [&] (auto&&...) { wasHandlerCalled = true; };

  m_manager.registerCommandHandler<TestCommandVoidParameters>("test-void", handler);
  m_manager.registerCommandHandler<TestCommandRequireName>("test-require-name", handler);
  setTopPrefix();

  auto testRegisterCommandHandler = [&] (const Name& commandName) {
    wasHandlerCalled = false;
    receiveInterest(makeControlCommandRequest(commandName));
  };

  testRegisterCommandHandler("/localhost/nfd/test-module/test-void");
  BOOST_CHECK(wasHandlerCalled);

  testRegisterCommandHandler("/localhost/nfd/test-module/test-require-name");
  BOOST_CHECK(!wasHandlerCalled);
}

BOOST_AUTO_TEST_CASE(RegisterStatusDataset)
{
  bool wasHandlerCalled = false;
  auto handler = [&] (auto&&...) { wasHandlerCalled = true; };

  m_manager.registerStatusDatasetHandler("test-status", handler);
  setTopPrefix();

  receiveInterest(*makeInterest("/localhost/nfd/test-module/test-status", true));
  BOOST_CHECK(wasHandlerCalled);
}

BOOST_AUTO_TEST_CASE(RegisterNotificationStream)
{
  auto post = m_manager.registerNotificationStream("test-notification");
  setTopPrefix();

  post(Block({0x82, 0x01, 0x02}));
  advanceClocks(1_ms);

  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(m_responses[0].getName(),
                    Name("/localhost/nfd/test-module/test-notification").appendSequenceNumber(0));
}

BOOST_AUTO_TEST_CASE(ExtractSigner)
{
  std::string signer = ManagerBase::extractSigner(Interest("/test/interest/unsigned"));
  BOOST_CHECK_EQUAL(signer, ""); // missing SignatureInfo

  auto signedV03 = makeControlCommandRequest("/test/interest/signed", {},
                                             ndn::security::SignedInterestFormat::V03);
  signer = ManagerBase::extractSigner(signedV03);
  BOOST_CHECK_EQUAL(signer,
                    m_keyChain.getPib().getIdentity(DEFAULT_COMMAND_SIGNER_IDENTITY)
                    .getDefaultKey().getDefaultCertificate().getName().toUri());

  signedV03.setSignatureInfo(ndn::SignatureInfo(tlv::SignatureSha256WithEcdsa));
  signer = ManagerBase::extractSigner(signedV03);
  BOOST_CHECK_EQUAL(signer, ""); // missing KeyLocator

  auto signedV02 = makeControlCommandRequest("/test/interest/signed", {},
                                             ndn::security::SignedInterestFormat::V02);
  signer = ManagerBase::extractSigner(signedV02);
  BOOST_CHECK_EQUAL(signer,
                    m_keyChain.getPib().getIdentity(DEFAULT_COMMAND_SIGNER_IDENTITY)
                    .getDefaultKey().getDefaultCertificate().getName().toUri());
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
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace nfd::tests
