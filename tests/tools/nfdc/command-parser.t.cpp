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

#include "nfdc/command-parser.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestCommandParser, BaseFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  CommandParser parser;
  ExecuteCommand dummyExecute = [] (ExecuteContext&) { return 0; };

  CommandDefinition defHelp("help", "");
  defHelp
    .addArg("noun", ArgValueType::STRING, Required::NO, Positional::YES)
    .addArg("verb", ArgValueType::STRING, Required::NO, Positional::YES);
  parser.addCommand(defHelp, dummyExecute, AVAILABLE_IN_ONE_SHOT);

  CommandDefinition defStatusShow("status", "show");
  parser.addCommand(defStatusShow, dummyExecute);
  parser.addAlias("status", "show", "list");
  BOOST_CHECK_THROW(parser.addAlias("status", "show2", "list"), std::out_of_range);

  CommandDefinition defRouteList("route", "list");
  defRouteList
    .addArg("nexthop", ArgValueType::FACE_ID_OR_URI, Required::NO, Positional::YES);
  parser.addCommand(defRouteList, dummyExecute);

  CommandDefinition defRouteAdd("route", "add");
  defRouteAdd
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES)
    .addArg("nexthop", ArgValueType::FACE_ID_OR_URI, Required::YES, Positional::YES);
  parser.addCommand(defRouteAdd, dummyExecute);
  parser.addAlias("route", "add", "add2");


  std::string noun, verb;
  CommandArguments ca;
  ExecuteCommand execute;

  std::tie(noun, verb, ca, execute) = parser.parse(
    std::vector<std::string>{"help"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "help");
  BOOST_CHECK_EQUAL(verb, "");

  std::tie(noun, verb, ca, execute) = parser.parse(
    std::vector<std::string>{"status"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "status");
  BOOST_CHECK_EQUAL(verb, "show");

  std::tie(noun, verb, ca, execute) = parser.parse(
    std::vector<std::string>{"route", "add", "/n", "300"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "route");
  BOOST_CHECK_EQUAL(verb, "add");
  BOOST_CHECK_EQUAL(boost::any_cast<Name>(ca.at("prefix")), "/n");
  BOOST_CHECK_EQUAL(boost::any_cast<uint64_t>(ca.at("nexthop")), 300);

  std::tie(noun, verb, ca, execute) = parser.parse(
    std::vector<std::string>{"route", "add2", "/n", "300"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "route");
  BOOST_CHECK_EQUAL(verb, "add");

  std::tie(noun, verb, ca, execute) = parser.parse(
    std::vector<std::string>{"route", "list", "400"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "route");
  BOOST_CHECK_EQUAL(verb, "list");
  BOOST_CHECK_EQUAL(boost::any_cast<uint64_t>(ca.at("nexthop")), 400);

  BOOST_CHECK_THROW(parser.parse(std::vector<std::string>{}, ParseMode::ONE_SHOT),
                    CommandParser::Error);
  BOOST_CHECK_THROW(parser.parse(std::vector<std::string>{"cant-help"}, ParseMode::ONE_SHOT),
                    CommandParser::Error);
  BOOST_CHECK_THROW(parser.parse(std::vector<std::string>{"status", "hide"}, ParseMode::ONE_SHOT),
                    CommandParser::Error);
  BOOST_CHECK_THROW(parser.parse(std::vector<std::string>{"route", "400"}, ParseMode::ONE_SHOT),
                    CommandParser::Error);
  BOOST_CHECK_THROW(parser.parse(std::vector<std::string>{"route", "add"}, ParseMode::ONE_SHOT),
                    CommandDefinition::Error);
}

BOOST_AUTO_TEST_SUITE_END() // TestCommandParser
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
