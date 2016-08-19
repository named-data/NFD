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

#include "nfdc/strategy-choice-module.hpp"

#include "module-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestStrategyChoiceModule, ModuleFixture<StrategyChoiceModule>)

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
  / strategy=/localhost/nfd/strategy/best-route/%FD%04
  /localhost strategy=/localhost/nfd/strategy/multicast/%FD%01
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(Status)
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
