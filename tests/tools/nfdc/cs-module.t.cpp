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

#include "nfdc/cs-module.hpp"

#include "status-fixture.hpp"
#include "execute-command-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_AUTO_TEST_SUITE(TestCsModule)

BOOST_FIXTURE_TEST_SUITE(ConfigCommand, ExecuteCommandFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  this->processInterest = [this] (const Interest& interest) {
    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/cs/config");
    BOOST_REQUIRE(req.hasCapacity());
    BOOST_CHECK_EQUAL(req.getCapacity(), 29850);
    BOOST_CHECK(req.hasFlagBit(ndn::nfd::BIT_CS_ENABLE_ADMIT));
    BOOST_CHECK_EQUAL(req.getFlagBit(ndn::nfd::BIT_CS_ENABLE_ADMIT), false);
    BOOST_CHECK(req.hasFlagBit(ndn::nfd::BIT_CS_ENABLE_SERVE));
    BOOST_CHECK_EQUAL(req.getFlagBit(ndn::nfd::BIT_CS_ENABLE_SERVE), true);

    ControlParameters resp(req);
    resp.unsetMask();
    this->succeedCommand(interest, resp);
  };

  this->execute("cs config admit off serve on capacity 29850");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("cs-config-updated capacity=29850 admit=off serve=on\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(NoUpdate)
{
  this->processInterest = [this] (const Interest& interest) {
    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/cs/config");
    BOOST_CHECK(!req.hasCapacity());
    BOOST_CHECK(!req.hasFlagBit(ndn::nfd::BIT_CS_ENABLE_ADMIT));
    BOOST_CHECK(!req.hasFlagBit(ndn::nfd::BIT_CS_ENABLE_SERVE));

    ControlParameters resp;
    resp.setCapacity(573599);
    resp.setFlagBit(ndn::nfd::BIT_CS_ENABLE_ADMIT, true, false);
    resp.setFlagBit(ndn::nfd::BIT_CS_ENABLE_SERVE, false, false);
    this->succeedCommand(interest, resp);
  };

  this->execute("cs config");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("cs-config-updated capacity=573599 admit=on serve=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(ErrorCommand)
{
  this->processInterest = nullptr; // no response to command

  this->execute("cs config capacity 19956");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when updating CS config: request timed out\n"));
}

BOOST_AUTO_TEST_SUITE_END() // ConfigCommand

const std::string STATUS_XML = stripXmlSpaces(R"XML(
  <cs>
    <capacity>31807</capacity>
    <admitEnabled/>
    <nEntries>16131</nEntries>
    <nHits>14363</nHits>
    <nMisses>27462</nMisses>
  </cs>
)XML");

const std::string STATUS_TEXT = std::string(R"TEXT(
CS information:
  capacity=31807
     admit=on
     serve=off
  nEntries=16131
     nHits=14363
   nMisses=27462
)TEXT").substr(1);

BOOST_FIXTURE_TEST_CASE(Status, StatusFixture<CsModule>)
{
  this->fetchStatus();
  CsInfo payload;
  payload.setCapacity(31807)
         .setEnableAdmit(true)
         .setEnableServe(false)
         .setNEntries(16131)
         .setNHits(14363)
         .setNMisses(27462);
  this->sendDataset("/localhost/nfd/cs/info", payload);
  this->prepareStatusOutput();

  BOOST_CHECK(statusXml.is_equal(STATUS_XML));
  BOOST_CHECK(statusText.is_equal(STATUS_TEXT));
}

BOOST_AUTO_TEST_SUITE_END() // TestCsModule
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
