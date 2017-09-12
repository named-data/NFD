/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "nfdc/strategy-choice-module.hpp"

#include "execute-command-fixture.hpp"
#include "status-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_AUTO_TEST_SUITE(TestStrategyChoiceModule)

class StrategyListFixture : public ExecuteCommandFixture
{
protected:
  bool
  respondStrategyChoiceDataset(const Interest& interest)
  {
    if (!Name("/localhost/nfd/strategy-choice/list").isPrefixOf(interest.getName())) {
      return false;
    }

    StrategyChoice entry1;
    entry1.setName("/");
    entry1.setStrategy("/strategyP/%FD%01");

    StrategyChoice entry2;
    entry2.setName("/52VRvpL9/Yqfut4TNHv");
    entry2.setStrategy("/strategyQ/%FD%02");

    this->sendDataset(interest.getName(), entry1, entry2);
    return true;
  }
};

BOOST_FIXTURE_TEST_SUITE(ListCommand, StrategyListFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(this->respondStrategyChoiceDataset(interest));
  };

  this->execute("strategy list");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("prefix=/ strategy=/strategyP/%FD%01\n"
                           "prefix=/52VRvpL9/Yqfut4TNHv strategy=/strategyQ/%FD%02\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(ErrorDataset)
{
  this->processInterest = nullptr; // no response to dataset or command

  this->execute("strategy list");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when fetching strategy choice dataset: Timeout\n"));
}

BOOST_AUTO_TEST_SUITE_END() // ListCommand

BOOST_FIXTURE_TEST_SUITE(ShowCommand, StrategyListFixture)

BOOST_AUTO_TEST_CASE(NormalDefaultStrategy)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(this->respondStrategyChoiceDataset(interest));
  };

  this->execute("strategy show /I1Ixgg0X");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("  prefix=/\n"
                           "strategy=/strategyP/%FD%01\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(NormalNonDefaultStrategy)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(this->respondStrategyChoiceDataset(interest));
  };

  this->execute("strategy show /52VRvpL9/Yqfut4TNHv/Y5gY7gom");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("  prefix=/52VRvpL9/Yqfut4TNHv\n"
                           "strategy=/strategyQ/%FD%02\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(ErrorDataset)
{
  this->processInterest = nullptr; // no response to dataset or command

  this->execute("strategy show /xVoIhNsJ");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when fetching strategy choice dataset: Timeout\n"));
}

BOOST_AUTO_TEST_SUITE_END() // ShowCommand

BOOST_FIXTURE_TEST_SUITE(SetCommand, ExecuteCommandFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  this->processInterest = [this] (const Interest& interest) {
    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/strategy-choice/set");
    BOOST_REQUIRE(req.hasName());
    BOOST_CHECK_EQUAL(req.getName(), "/VBXSJg3m/XYs81ARNhx");
    BOOST_REQUIRE(req.hasStrategy());
    BOOST_CHECK_EQUAL(req.getStrategy(), "/strategyP");

    ControlParameters resp;
    resp.setName("/VBXSJg3m/XYs81ARNhx");
    resp.setStrategy("/strategyP/%FD%05");
    this->succeedCommand(interest, resp);
  };

  this->execute("strategy set /VBXSJg3m/XYs81ARNhx /strategyP");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("strategy-set prefix=/VBXSJg3m/XYs81ARNhx strategy=/strategyP/%FD%05\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(UnknownStrategy)
{
  this->processInterest = [this] (const Interest& interest) {
    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/strategy-choice/set");

    ControlParameters resp;
    this->failCommand(interest, 404, "strategy not found");
  };

  this->execute("strategy set /zezRSP1I /strategyQ");
  BOOST_CHECK_EQUAL(exitCode, 7);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Unknown strategy: /strategyQ\n"));
}

BOOST_AUTO_TEST_CASE(ErrorCommand)
{
  this->processInterest = nullptr; // no response to command

  this->execute("strategy set /LJdNFfQJ8 /strategyP");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when setting strategy: request timed out\n"));
}

BOOST_AUTO_TEST_SUITE_END() // SetCommand

BOOST_FIXTURE_TEST_SUITE(UnsetCommand, ExecuteCommandFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  this->processInterest = [this] (const Interest& interest) {
    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/strategy-choice/unset");
    BOOST_REQUIRE(req.hasName());
    BOOST_CHECK_EQUAL(req.getName(), "/CFVecryP");

    ControlParameters resp;
    resp.setName("/CFVecryP");
    this->succeedCommand(interest, resp);
  };

  this->execute("strategy unset /CFVecryP");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("strategy-unset prefix=/CFVecryP\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(CannotUnsetDefault)
{
  this->processInterest = [] (const Interest&) {
    BOOST_ERROR("unexpected command");
  };

  this->execute("strategy unset /");
  BOOST_CHECK_EQUAL(exitCode, 2);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Unsetting default strategy is prohibited\n"));
}

BOOST_AUTO_TEST_CASE(ErrorCommand)
{
  this->processInterest = nullptr; // no response to command

  this->execute("strategy unset /GQcEsG96");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when unsetting strategy: request timed out\n"));
}

BOOST_AUTO_TEST_SUITE_END() // UnsetCommand

const std::string STATUS_XML = stripXmlSpaces(R"XML(
  <strategyChoices>
    <strategyChoice>
      <namespace>/</namespace>
      <strategy>
        <name>/localhost/nfd/strategy/best-route/%FD%04</name>
      </strategy>
    </strategyChoice>
    <strategyChoice>
      <namespace>/localhost</namespace>
      <strategy>
        <name>/localhost/nfd/strategy/multicast/%FD%01</name>
      </strategy>
    </strategyChoice>
  </strategyChoices>
)XML");

const std::string STATUS_TEXT = std::string(R"TEXT(
Strategy choices:
  prefix=/ strategy=/localhost/nfd/strategy/best-route/%FD%04
  prefix=/localhost strategy=/localhost/nfd/strategy/multicast/%FD%01
)TEXT").substr(1);

BOOST_FIXTURE_TEST_CASE(Status, StatusFixture<StrategyChoiceModule>)
{
  this->fetchStatus();
  StrategyChoice payload1;
  payload1.setName("/")
          .setStrategy("/localhost/nfd/strategy/best-route/%FD%04");
  StrategyChoice payload2;
  payload2.setName("/localhost")
          .setStrategy("/localhost/nfd/strategy/multicast/%FD%01");
  this->sendDataset("/localhost/nfd/strategy-choice/list", payload1, payload2);
  this->prepareStatusOutput();

  BOOST_CHECK(statusXml.is_equal(STATUS_XML));
  BOOST_CHECK(statusText.is_equal(STATUS_TEXT));
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyChoiceModule
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
