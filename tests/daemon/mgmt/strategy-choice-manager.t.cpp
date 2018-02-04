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

#include "mgmt/strategy-choice-manager.hpp"
#include "table/strategy-choice.hpp"

#include "nfd-manager-common-fixture.hpp"
#include "../fw/dummy-strategy.hpp"
#include <ndn-cxx/mgmt/nfd/strategy-choice.hpp>

namespace nfd {
namespace tests {

class StrategyChoiceManagerFixture : public NfdManagerCommonFixture
{
public:
  StrategyChoiceManagerFixture()
    : sc(m_forwarder.getStrategyChoice())
    , manager(sc, m_dispatcher, *m_authenticator)
    , strategyNameP("/strategy-choice-manager-P/%FD%02")
  {
    VersionedDummyStrategy<2>::registerAs(strategyNameP);

    setTopPrefix();
    setPrivilege("strategy-choice");
  }

public:
  /** \return whether exact-match StrategyChoice entry exists
   */
  bool
  hasEntry(const Name& name) const
  {
    return sc.get(name).first;
  }

  /** \return strategy instance name from an exact-match StrategyChoice entry
   */
  Name
  getInstanceName(const Name& name) const
  {
    bool hasEntry = false;
    Name instanceName;
    std::tie(hasEntry, instanceName) = sc.get(name);
    return hasEntry ?
           instanceName :
           Name("/no-StrategyChoice-entry-at").append(name);
  }

protected:
  StrategyChoice& sc;
  StrategyChoiceManager manager;

  const Name strategyNameP;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestStrategyChoiceManager, StrategyChoiceManagerFixture)

BOOST_AUTO_TEST_CASE(SetSuccess)
{
  ControlParameters reqParams;
  reqParams.setName("/A")
           .setStrategy(strategyNameP.getPrefix(-1)); // use unversioned strategy name in request
  auto req = makeControlCommandRequest("/localhost/nfd/strategy-choice/set", reqParams);
  receiveInterest(req);

  ControlParameters expectedParams;
  expectedParams.setName("/A")
                .setStrategy(strategyNameP); // response should have versioned strategy name
  ControlResponse expectedResp;
  expectedResp.setCode(200)
              .setText("OK")
              .setBody(expectedParams.wireEncode());
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), expectedResp),
                    CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(getInstanceName("/A"), strategyNameP);

  // Strategy versioning and parameters are not tested here because they are covered by
  // Table/TestStrategyChoice test suite.
}

BOOST_AUTO_TEST_CASE(SetUnknownStrategy)
{
  ControlParameters reqParams;
  reqParams.setName("/A")
           .setStrategy("/strategy-choice-manager-unknown");
  auto req = makeControlCommandRequest("/localhost/nfd/strategy-choice/set", reqParams);
  receiveInterest(req);

  ControlResponse expectedResp;
  expectedResp.setCode(404)
              .setText("Strategy not registered");
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), expectedResp),
                    CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(hasEntry("/A"), false);
}

BOOST_AUTO_TEST_CASE(SetNameTooLong)
{
  Name prefix;
  while (prefix.size() <= FIB_MAX_DEPTH) {
    prefix.append("A");
  }
  ControlParameters reqParams;
  reqParams.setName(prefix)
           .setStrategy(strategyNameP);
  auto req = makeControlCommandRequest("/localhost/nfd/strategy-choice/set", reqParams);
  receiveInterest(req);

  ControlResponse expectedResp;
  expectedResp.setCode(414)
              .setText("Prefix has too many components (limit is " +
                       to_string(NameTree::getMaxDepth()) + ")");
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), expectedResp),
                    CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(hasEntry(prefix), false);
}

BOOST_AUTO_TEST_CASE(UnsetSuccess)
{
  auto insertRes = sc.insert("/A", strategyNameP);
  BOOST_REQUIRE(insertRes);

  ControlParameters reqParams;
  reqParams.setName("/A");
  auto req = makeControlCommandRequest("/localhost/nfd/strategy-choice/unset", reqParams);
  receiveInterest(req);

  ControlParameters expectedParams(reqParams);
  ControlResponse expectedResp;
  expectedResp.setCode(200)
              .setText("OK")
              .setBody(expectedParams.wireEncode());
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), expectedResp),
                    CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(hasEntry("/A"), false);
}

BOOST_AUTO_TEST_CASE(UnsetNoop)
{
  ControlParameters reqParams;
  reqParams.setName("/A");
  auto req = makeControlCommandRequest("/localhost/nfd/strategy-choice/unset", reqParams);
  receiveInterest(req);

  ControlParameters expectedParams(reqParams);
  ControlResponse expectedResp;
  expectedResp.setCode(200)
              .setText("OK")
              .setBody(expectedParams.wireEncode());
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), expectedResp),
                    CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(hasEntry("/A"), false);
}

BOOST_AUTO_TEST_CASE(UnsetRootForbidden)
{
  ControlParameters reqParams;
  reqParams.setName("/");
  auto req = makeControlCommandRequest("/localhost/nfd/strategy-choice/unset", reqParams);
  receiveInterest(req);

  ControlResponse expectedResp;
  expectedResp.setCode(400)
              .setText("failed in validating parameters");
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), expectedResp),
                    CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(hasEntry("/"), true);
}

BOOST_AUTO_TEST_CASE(StrategyChoiceDataset)
{
  std::map<Name, Name> expected; // namespace => strategy instance name
  for (const strategy_choice::Entry& entry : sc) {
    expected[entry.getPrefix()] = entry.getStrategyInstanceName();
  }

  for (int i = expected.size(); i < 1024; ++i) {
    Name name("/SC");
    name.appendNumber(i);
    Name strategy = DummyStrategy::getStrategyName(i);

    auto insertRes = sc.insert(name, strategy);
    BOOST_CHECK(insertRes);
    expected[name] = strategy;
  }

  receiveInterest(Interest("/localhost/nfd/strategy-choice/list"));
  Block dataset = concatenateResponses();
  dataset.parse();
  BOOST_CHECK_EQUAL(dataset.elements_size(), expected.size());

  for (auto i = dataset.elements_begin(); i != dataset.elements_end(); ++i) {
    ndn::nfd::StrategyChoice record(*i);
    auto found = expected.find(record.getName());
    if (found == expected.end()) {
      BOOST_ERROR("record has unexpected namespace " << record.getName());
    }
    else {
      BOOST_CHECK_MESSAGE(record.getStrategy() == found->second,
        "record for " << record.getName() << " has wrong strategy " << record.getStrategy() <<
        ", should be " << found->second);
      expected.erase(found);
    }
  }

  for (const auto& pair : expected) {
    BOOST_ERROR("record for " << pair.first << " is missing");
  }
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyChoiceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
