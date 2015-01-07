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

#include "mgmt/strategy-choice-manager.hpp"
#include "face/face.hpp"
#include "mgmt/internal-face.hpp"
#include "table/name-tree.hpp"
#include "table/strategy-choice.hpp"
#include "fw/forwarder.hpp"
#include "fw/strategy.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "tests/daemon/fw/dummy-strategy.hpp"

#include <ndn-cxx/management/nfd-strategy-choice.hpp>

#include "tests/test-common.hpp"
#include "validation-common.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("MgmtStrategyChoiceManager");

class StrategyChoiceManagerFixture : protected BaseFixture
{
public:

  StrategyChoiceManagerFixture()
    : m_strategyChoice(m_forwarder.getStrategyChoice())
    , m_face(make_shared<InternalFace>())
    , m_manager(m_strategyChoice, m_face, m_keyChain)
    , m_callbackFired(false)
  {
    m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                        "/localhost/nfd/strategy/test-strategy-a"));
    m_strategyChoice.insert("ndn:/", "/localhost/nfd/strategy/test-strategy-a");
  }

  virtual
  ~StrategyChoiceManagerFixture()
  {

  }

  void
  validateControlResponseCommon(const Data& response,
                                const Name& expectedName,
                                uint32_t expectedCode,
                                const std::string& expectedText,
                                ControlResponse& control)
  {
    m_callbackFired = true;
    Block controlRaw = response.getContent().blockFromValue();

    control.wireDecode(controlRaw);

    NFD_LOG_DEBUG("received control response"
                  << " Name: " << response.getName()
                  << " code: " << control.getCode()
                  << " text: " << control.getText());

    BOOST_CHECK_EQUAL(response.getName(), expectedName);
    BOOST_CHECK_EQUAL(control.getCode(), expectedCode);
    BOOST_CHECK_EQUAL(control.getText(), expectedText);
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText)
  {
    ControlResponse control;
    validateControlResponseCommon(response, expectedName,
                                  expectedCode, expectedText, control);

    if (!control.getBody().empty())
      {
        BOOST_FAIL("found unexpected control response body");
      }
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText,
                          const Block& expectedBody)
  {
    ControlResponse control;
    validateControlResponseCommon(response, expectedName,
                                  expectedCode, expectedText, control);

    BOOST_REQUIRE(!control.getBody().empty());
    BOOST_REQUIRE(control.getBody().value_size() == expectedBody.value_size());

    BOOST_CHECK(memcmp(control.getBody().value(), expectedBody.value(),
                       expectedBody.value_size()) == 0);

  }

  void
  validateList(const Data& data, const ndn::nfd::StrategyChoice& expectedChoice)
  {
    m_callbackFired = true;
    ndn::nfd::StrategyChoice choice(data.getContent().blockFromValue());
    BOOST_CHECK_EQUAL(choice.getStrategy(), expectedChoice.getStrategy());
    BOOST_CHECK_EQUAL(choice.getName(), expectedChoice.getName());
  }

  bool
  didCallbackFire()
  {
    return m_callbackFired;
  }

  void
  resetCallbackFired()
  {
    m_callbackFired = false;
  }

  shared_ptr<InternalFace>&
  getFace()
  {
    return m_face;
  }

  StrategyChoiceManager&
  getManager()
  {
    return m_manager;
  }

  StrategyChoice&
  getStrategyChoice()
  {
    return m_strategyChoice;
  }

  void
  addInterestRule(const std::string& regex,
                  ndn::IdentityCertificate& certificate)
  {
    m_manager.addInterestRule(regex, certificate);
  }

protected:
  Forwarder m_forwarder;
  StrategyChoice& m_strategyChoice;
  shared_ptr<InternalFace> m_face;
  StrategyChoiceManager m_manager;
  ndn::KeyChain m_keyChain;

private:
  bool m_callbackFired;
};

class AllStrategiesFixture : public StrategyChoiceManagerFixture
{
public:
  AllStrategiesFixture()
  {
    m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                        "/localhost/nfd/strategy/test-strategy-b"));

    const Name strategyCVersion1("/localhost/nfd/strategy/test-strategy-c/%FD%01");
    m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                        strategyCVersion1));

    const Name strategyCVersion2("/localhost/nfd/strategy/test-strategy-c/%FD%02");
    m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                        strategyCVersion2));
  }

  virtual
  ~AllStrategiesFixture()
  {

  }
};

template <typename T> class AuthorizedCommandFixture : public CommandFixture<T>
{
public:
  AuthorizedCommandFixture()
  {
    const std::string regex = "^<localhost><nfd><strategy-choice>";
    T::addInterestRule(regex, *CommandFixture<T>::m_certificate);
  }

  virtual
  ~AuthorizedCommandFixture()
  {

  }
};

BOOST_FIXTURE_TEST_SUITE(MgmtStrategyChoiceManager,
                         AuthorizedCommandFixture<AllStrategiesFixture>)

BOOST_FIXTURE_TEST_CASE(ShortName, AllStrategiesFixture)
{
  shared_ptr<Interest> command(make_shared<Interest>("/localhost/nfd/strategy-choice"));

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getFace()->sendInterest(*command);
  g_io.run_one();

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(MalformedCommmand, AllStrategiesFixture)
{
  shared_ptr<Interest> command(make_shared<Interest>("/localhost/nfd/strategy-choice"));

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getManager().onStrategyChoiceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(UnsignedCommand, AllStrategiesFixture)
{
  ControlParameters parameters;
  parameters.setName("/test");
  parameters.setStrategy("/localhost/nfd/strategy/best-route");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 401, "Signature required");
  });

  getManager().onStrategyChoiceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(UnauthorizedCommand,
                        UnauthorizedCommandFixture<StrategyChoiceManagerFixture>)
{
  ControlParameters parameters;
  parameters.setName("/test");
  parameters.setStrategy("/localhost/nfd/strategy/best-route");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 403, "Unauthorized command");
  });

  getManager().onStrategyChoiceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(UnsupportedVerb)
{
  ControlParameters parameters;
  parameters.setName("/test");
  parameters.setStrategy("/localhost/nfd/strategy/test-strategy-b");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("unsupported");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 501, "Unsupported command");
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(BadOptionParse)
{
  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append("NotReallyParameters");

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(SetStrategies)
{
  ControlParameters parameters;
  parameters.setName("/test");
  parameters.setStrategy("/localhost/nfd/strategy/test-strategy-b");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  getFace()->onReceiveData.connect([this, command, encodedParameters] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  200, "Success", encodedParameters);
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  fw::Strategy& strategy = getStrategyChoice().findEffectiveStrategy("/test");
  BOOST_REQUIRE_EQUAL(strategy.getName(), "/localhost/nfd/strategy/test-strategy-b");
}

BOOST_AUTO_TEST_CASE(SetStrategySpecifiedVersion)
{
  ControlParameters parameters;
  parameters.setName("/test");
  parameters.setStrategy("/localhost/nfd/strategy/test-strategy-c/%FD%01");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  getFace()->onReceiveData.connect([this, command, encodedParameters] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  200, "Success", encodedParameters);
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  fw::Strategy& strategy = getStrategyChoice().findEffectiveStrategy("/test");
  BOOST_REQUIRE_EQUAL(strategy.getName(), "/localhost/nfd/strategy/test-strategy-c/%FD%01");
}

BOOST_AUTO_TEST_CASE(SetStrategyLatestVersion)
{
  ControlParameters parameters;
  parameters.setName("/test");
  parameters.setStrategy("/localhost/nfd/strategy/test-strategy-c");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  ControlParameters responseParameters;
  responseParameters.setName("/test");
  responseParameters.setStrategy("/localhost/nfd/strategy/test-strategy-c/%FD%02");

  getFace()->onReceiveData.connect([this, command, responseParameters] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  200, "Success", responseParameters.wireEncode());
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  fw::Strategy& strategy = getStrategyChoice().findEffectiveStrategy("/test");
  BOOST_REQUIRE_EQUAL(strategy.getName(), "/localhost/nfd/strategy/test-strategy-c/%FD%02");
}

BOOST_AUTO_TEST_CASE(SetStrategiesMissingName)
{
  ControlParameters parameters;
  parameters.setStrategy("/localhost/nfd/strategy/test-strategy-b");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(SetStrategiesMissingStrategy)
{
  ControlParameters parameters;
  parameters.setName("/test");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  fw::Strategy& strategy = getStrategyChoice().findEffectiveStrategy("/test");
  BOOST_REQUIRE_EQUAL(strategy.getName(), "/localhost/nfd/strategy/test-strategy-a");
}

BOOST_AUTO_TEST_CASE(SetUnsupportedStrategy)
{
  ControlParameters parameters;
  parameters.setName("/test");
  parameters.setStrategy("/localhost/nfd/strategy/unit-test-doesnotexist");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("set");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 504, "Unsupported strategy");
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  fw::Strategy& strategy = getStrategyChoice().findEffectiveStrategy("/test");
  BOOST_CHECK_EQUAL(strategy.getName(), "/localhost/nfd/strategy/test-strategy-a");
}

class DefaultStrategyOnlyFixture : public StrategyChoiceManagerFixture
{
public:
  DefaultStrategyOnlyFixture()
    : StrategyChoiceManagerFixture()
  {

  }

  virtual
  ~DefaultStrategyOnlyFixture()
  {

  }
};


/// \todo I'm not sure this code branch (code 405) can happen. The manager tests for the strategy first and will return 504.
// BOOST_FIXTURE_TEST_CASE(SetNotInstalled, DefaultStrategyOnlyFixture)
// {
//   BOOST_REQUIRE(!getStrategyChoice().hasStrategy("/localhost/nfd/strategy/test-strategy-b"));
//   ControlParameters parameters;
//   parameters.setName("/test");
//   parameters.setStrategy("/localhost/nfd/strategy/test-strategy-b");

//   Block encodedParameters(parameters.wireEncode());

//   Name commandName("/localhost/nfd/strategy-choice");
//   commandName.append("set");
//   commandName.append(encodedParameters);

//   shared_ptr<Interest> command(make_shared<Interest>(commandName));
//   generateCommand(*command);

//   getFace()->onReceiveData +=
//     bind(&StrategyChoiceManagerFixture::validateControlResponse, this, _1,
//          command->getName(), 405, "Strategy not installed");

//   getManager().onValidatedStrategyChoiceRequest(command);

//   BOOST_REQUIRE(didCallbackFire());
//   fw::Strategy& strategy = getStrategyChoice().findEffectiveStrategy("/test");
//   BOOST_CHECK_EQUAL(strategy.getName(), "/localhost/nfd/strategy/test-strategy-a");
// }

BOOST_AUTO_TEST_CASE(Unset)
{
  ControlParameters parameters;
  parameters.setName("/test");

  BOOST_REQUIRE(m_strategyChoice.insert("/test", "/localhost/nfd/strategy/test-strategy-b"));
  BOOST_REQUIRE_EQUAL(m_strategyChoice.findEffectiveStrategy("/test").getName(),
                      "/localhost/nfd/strategy/test-strategy-b");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("unset");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command, encodedParameters] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  200, "Success", encodedParameters);
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());

  BOOST_CHECK_EQUAL(m_strategyChoice.findEffectiveStrategy("/test").getName(),
                    "/localhost/nfd/strategy/test-strategy-a");
}

BOOST_AUTO_TEST_CASE(UnsetRoot)
{
  ControlParameters parameters;
  parameters.setName("/");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("unset");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  403, "Cannot unset root prefix strategy");
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());

  BOOST_CHECK_EQUAL(m_strategyChoice.findEffectiveStrategy("/test").getName(),
                    "/localhost/nfd/strategy/test-strategy-a");
}

BOOST_AUTO_TEST_CASE(UnsetMissingName)
{
  ControlParameters parameters;

  BOOST_REQUIRE(m_strategyChoice.insert("/test", "/localhost/nfd/strategy/test-strategy-b"));
  BOOST_REQUIRE_EQUAL(m_strategyChoice.findEffectiveStrategy("/test").getName(),
                      "/localhost/nfd/strategy/test-strategy-b");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/strategy-choice");
  commandName.append("unset");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getManager().onValidatedStrategyChoiceRequest(command);

  BOOST_REQUIRE(didCallbackFire());

  BOOST_CHECK_EQUAL(m_strategyChoice.findEffectiveStrategy("/test").getName(),
                    "/localhost/nfd/strategy/test-strategy-b");
}

BOOST_AUTO_TEST_CASE(Publish)
{
  Name commandName("/localhost/nfd/strategy-choice/list");
  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  ndn::nfd::StrategyChoice expectedChoice;
  expectedChoice.setStrategy("/localhost/nfd/strategy/test-strategy-a");
  expectedChoice.setName("/");

  getFace()->onReceiveData.connect(bind(&StrategyChoiceManagerFixture::validateList,
                                        this, _1, expectedChoice));

  m_manager.onStrategyChoiceRequest(*command);
  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
