/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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
#include "nfd-manager-common-fixture.hpp"

#include "face/face.hpp"
#include "face/internal-face.hpp"
#include "table/name-tree.hpp"
#include "table/strategy-choice.hpp"
#include "fw/strategy.hpp"

#include "tests/daemon/face/dummy-face.hpp"
#include "tests/daemon/fw/dummy-strategy.hpp"
#include "tests/daemon/fw/install-strategy.hpp"

#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/mgmt/nfd/strategy-choice.hpp>

namespace nfd {
namespace tests {

class StrategyChoiceManagerFixture : public NfdManagerCommonFixture
{
public:
  StrategyChoiceManagerFixture()
    : m_strategyChoice(m_forwarder.getStrategyChoice())
    , m_manager(m_strategyChoice, m_dispatcher, *m_authenticator)
  {
    setTopPrefix();
    setPrivilege("strategy-choice");
  }

public:
  void
  installStrategy(const Name& strategyName)
  {
    install<DummyStrategy>(m_forwarder, strategyName);
  }

  const Name&
  findStrategy(const Name& name)
  {
    return m_strategyChoice.findEffectiveStrategy(name).getName();
  }

  ControlParameters
  makeParameters(const Name& name, const Name& strategy)
  {
    return ControlParameters().setName(name).setStrategy(strategy);
  }

protected:
  StrategyChoice& m_strategyChoice;
  StrategyChoiceManager m_manager;
};

BOOST_FIXTURE_TEST_SUITE(Mgmt, StrategyChoiceManagerFixture)
BOOST_AUTO_TEST_SUITE(TestStrategyChoiceManager)

BOOST_AUTO_TEST_CASE(SetStrategy)
{
  auto testSetStrategy = [this] (const ControlParameters& parameters) -> Name {
    m_responses.clear();
    auto command = makeControlCommandRequest("/localhost/nfd/strategy-choice/set", parameters);
    receiveInterest(command);
    return command->getName();
  };

  installStrategy("/localhost/nfd/strategy/test-strategy-a");
  installStrategy("/localhost/nfd/strategy/test-strategy-c/%FD%01"); // version 1
  installStrategy("/localhost/nfd/strategy/test-strategy-c/%FD%02"); // version 2

  auto parametersA  = makeParameters("test", "/localhost/nfd/strategy/test-strategy-a");
  auto parametersB  = makeParameters("test", "/localhost/nfd/strategy/test-strategy-b");
  auto parametersC1 = makeParameters("test", "/localhost/nfd/strategy/test-strategy-c/%FD%01");
  auto parametersC  = makeParameters("test", "/localhost/nfd/strategy/test-strategy-c");

  auto commandNameA = testSetStrategy(parametersA); // succeed
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, commandNameA, makeResponse(200, "OK", parametersA)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(findStrategy("/test"), "/localhost/nfd/strategy/test-strategy-a");

  auto commandNameB = testSetStrategy(parametersB); // not installed
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, commandNameB, ControlResponse(504, "Unsupported strategy")),
                    CheckResponseResult::OK);

  auto commandNameC1 = testSetStrategy(parametersC1); // specified version
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, commandNameC1, makeResponse(200, "OK", parametersC1)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(findStrategy("/test"), "/localhost/nfd/strategy/test-strategy-c/%FD%01");

  auto commandNameC = testSetStrategy(parametersC); // latest version
  parametersC.setStrategy("/localhost/nfd/strategy/test-strategy-c/%FD%02"); // change to latest
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, commandNameC, makeResponse(200, "OK", parametersC)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(findStrategy("/test"), "/localhost/nfd/strategy/test-strategy-c/%FD%02");
}

BOOST_AUTO_TEST_CASE(UnsetStrategy)
{
  auto testUnsetStrategy = [this] (const ControlParameters& parameters) -> Name {
    m_responses.clear();
    auto command = makeControlCommandRequest("/localhost/nfd/strategy-choice/unset", parameters);
    receiveInterest(command);
    return command->getName();
  };

  installStrategy("/localhost/nfd/strategy/test-strategy-a");
  installStrategy("/localhost/nfd/strategy/test-strategy-b");
  installStrategy("/localhost/nfd/strategy/test-strategy-c");

  BOOST_CHECK(m_strategyChoice.insert("ndn:/", "/localhost/nfd/strategy/test-strategy-a")); // root
  BOOST_CHECK(m_strategyChoice.insert("/test", "/localhost/nfd/strategy/test-strategy-b")); // test
  BOOST_CHECK_EQUAL(findStrategy("/"), "/localhost/nfd/strategy/test-strategy-a");

  auto parametersRoot = ControlParameters().setName("ndn:/"); // root prefix
  auto parametersNone = ControlParameters().setName("/none"); // no such entry
  auto parametersTest = ControlParameters().setName("/test"); // has such entry

  BOOST_CHECK_EQUAL(findStrategy("/"), "/localhost/nfd/strategy/test-strategy-a"); // root
  auto commandRootName = testUnsetStrategy(parametersRoot);
  auto expectedResponse = ControlResponse(400, "failed in validating parameters");
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, commandRootName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(findStrategy("/"), "/localhost/nfd/strategy/test-strategy-a"); // keep as root

  BOOST_CHECK_EQUAL(findStrategy("/none"), "/localhost/nfd/strategy/test-strategy-a"); // root
  auto commandNoneName = testUnsetStrategy(parametersNone);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, commandNoneName, makeResponse(200, "OK", parametersNone)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(findStrategy("/none"), "/localhost/nfd/strategy/test-strategy-a"); // root

  BOOST_CHECK_EQUAL(findStrategy("/test"), "/localhost/nfd/strategy/test-strategy-b"); // self
  auto commandTestName = testUnsetStrategy(parametersTest);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, commandTestName, makeResponse(200, "OK", parametersTest)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(findStrategy("/test"), "/localhost/nfd/strategy/test-strategy-a"); // parent
}

// @todo Remove when ndn::nfd::StrategyChoice implements operator!= and operator<<
class StrategyChoice : public ndn::nfd::StrategyChoice
{
public:
  StrategyChoice() = default;

  StrategyChoice(const ndn::nfd::StrategyChoice& entry)
    : ndn::nfd::StrategyChoice(entry)
  {
  }
};

bool
operator!=(const StrategyChoice& left, const StrategyChoice& right)
{
  return left.getName() != right.getName() || left.getStrategy() != right.getStrategy();
}

std::ostream&
operator<<(std::ostream &os, const StrategyChoice& entry)
{
  os << "[ " << entry.getName() << ", " << entry.getStrategy() << " ]";
  return os;
}

BOOST_AUTO_TEST_CASE(ListChoices)
{
  size_t nPreInsertedStrategies = m_strategyChoice.size(); // the best-route strategy
  std::set<Name> actualNames, actualStrategies;
  for (auto&& entry : m_strategyChoice) {
    actualNames.insert(entry.getPrefix());
    actualStrategies.insert(entry.getStrategyName());
  }

  size_t nEntries = 1024;
  for (size_t i = 0 ; i < nEntries ; i++) {
    auto name = Name("test-name").appendSegment(i);
    auto strategy = Name("test-strategy").appendSegment(ndn::random::generateWord64());
    auto entry = ndn::nfd::StrategyChoice().setName(name).setStrategy(strategy);
    actualNames.insert(name);
    actualStrategies.insert(strategy);
    installStrategy(strategy);
    m_strategyChoice.insert(name, strategy);
  }
  nEntries += nPreInsertedStrategies;

  receiveInterest(makeInterest("/localhost/nfd/strategy-choice/list"));

  Block content;
  BOOST_CHECK_NO_THROW(content = concatenateResponses());
  BOOST_CHECK_NO_THROW(content.parse());
  BOOST_CHECK_EQUAL(content.elements().size(), nEntries);

  std::vector<StrategyChoice> receivedRecords, expectedRecords;
  for (size_t idx = 0; idx < nEntries; ++idx) {
    BOOST_TEST_MESSAGE("processing element: " << idx);

    StrategyChoice decodedEntry;
    BOOST_REQUIRE_NO_THROW(decodedEntry.wireDecode(content.elements()[idx]));
    receivedRecords.push_back(decodedEntry);

    actualNames.erase(decodedEntry.getName());
    actualStrategies.erase(decodedEntry.getStrategy());

    auto result = m_strategyChoice.get(decodedEntry.getName());
    BOOST_REQUIRE(result.first);

    auto record = StrategyChoice().setName(decodedEntry.getName()).setStrategy(result.second);
    expectedRecords.push_back(record);
  }

  BOOST_CHECK_EQUAL(actualNames.size(), 0);
  BOOST_CHECK_EQUAL(actualStrategies.size(), 0);

  BOOST_CHECK_EQUAL_COLLECTIONS(receivedRecords.begin(), receivedRecords.end(),
                                expectedRecords.begin(), expectedRecords.end());
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyChoiceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
