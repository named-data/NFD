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

#include "nfdc/command-parser.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestCommandParser, BaseFixture)

BOOST_AUTO_TEST_CASE(PrintAvailableIn)
{
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(AVAILABLE_IN_NONE), "hidden");
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(AVAILABLE_IN_ONE_SHOT), "one-shot|hidden");
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(AVAILABLE_IN_BATCH), "batch|hidden");
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(AVAILABLE_IN_HELP), "none");
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(AVAILABLE_IN_ALL), "one-shot|batch");
}

BOOST_AUTO_TEST_CASE(PrintParseMode)
{
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(ParseMode::ONE_SHOT), "one-shot");
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(ParseMode::BATCH), "batch");
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(static_cast<ParseMode>(0xFF)), "255");
}

BOOST_AUTO_TEST_CASE(Basic)
{
  CommandParser parser;
  ExecuteCommand dummyExecute = [] (ExecuteContext&) { BOOST_ERROR("should not be called"); };

  BOOST_CHECK(parser.listCommands("", ParseMode::ONE_SHOT).empty());

  CommandDefinition defFoo("foo", "");
  parser.addCommand(defFoo, dummyExecute, AVAILABLE_IN_ONE_SHOT | AVAILABLE_IN_HELP);

  CommandDefinition defStatusShow("status", "show");
  parser.addCommand(defStatusShow, dummyExecute);
  parser.addAlias("status", "show", "");
  parser.addAlias("status", "show", "list");
  BOOST_CHECK_THROW(parser.addAlias("status", "show2", "list"), std::out_of_range);

  CommandDefinition defRouteList("route", "list");
  defRouteList
    .addArg("nexthop", ArgValueType::FACE_ID_OR_URI, Required::NO, Positional::YES);
  parser.addCommand(defRouteList, dummyExecute);
  parser.addAlias("route", "list", "");

  CommandDefinition defRouteAdd("route", "add");
  defRouteAdd
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES)
    .addArg("nexthop", ArgValueType::FACE_ID_OR_URI, Required::YES, Positional::YES);
  parser.addCommand(defRouteAdd, dummyExecute);
  parser.addAlias("route", "add", "add2");

  CommandDefinition defHidden("hidden", "");
  parser.addCommand(defHidden, dummyExecute, AVAILABLE_IN_BATCH);

  BOOST_CHECK_EQUAL(parser.listCommands("", ParseMode::ONE_SHOT).size(), 4);
  BOOST_CHECK_EQUAL(parser.listCommands("", ParseMode::BATCH).size(), 3);
  BOOST_CHECK_EQUAL(parser.listCommands("route", ParseMode::ONE_SHOT).size(), 2);
  BOOST_CHECK_EQUAL(parser.listCommands("unknown", ParseMode::ONE_SHOT).size(), 0);

  std::string noun, verb;
  CommandArguments ca;
  ExecuteCommand execute;

  std::tie(noun, verb, ca, execute) = parser.parse({"foo"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "foo");
  BOOST_CHECK_EQUAL(verb, "");

  std::tie(noun, verb, ca, execute) = parser.parse({"status"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "status");
  BOOST_CHECK_EQUAL(verb, "show");

  std::tie(noun, verb, ca, execute) = parser.parse({"status", "list"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "status");
  BOOST_CHECK_EQUAL(verb, "show");

  std::tie(noun, verb, ca, execute) = parser.parse({"route", "add", "/n", "300"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "route");
  BOOST_CHECK_EQUAL(verb, "add");
  BOOST_CHECK_EQUAL(boost::any_cast<Name>(ca.at("prefix")), "/n");
  BOOST_CHECK_EQUAL(boost::any_cast<uint64_t>(ca.at("nexthop")), 300);

  std::tie(noun, verb, ca, execute) = parser.parse({"route", "add2", "/n", "300"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "route");
  BOOST_CHECK_EQUAL(verb, "add");

  std::tie(noun, verb, ca, execute) = parser.parse({"route", "list", "400"}, ParseMode::ONE_SHOT);
  BOOST_CHECK_EQUAL(noun, "route");
  BOOST_CHECK_EQUAL(verb, "list");
  BOOST_CHECK_EQUAL(boost::any_cast<uint64_t>(ca.at("nexthop")), 400);

  BOOST_CHECK_THROW(parser.parse({}, ParseMode::ONE_SHOT),
                    CommandParser::NoSuchCommandError);
  BOOST_CHECK_THROW(parser.parse({"bar"}, ParseMode::ONE_SHOT),
                    CommandParser::NoSuchCommandError);
  BOOST_CHECK_THROW(parser.parse({"status", "hide"}, ParseMode::ONE_SHOT),
                    CommandParser::NoSuchCommandError);
  BOOST_CHECK_THROW(parser.parse({"status", "show", "something"}, ParseMode::ONE_SHOT),
                    CommandDefinition::Error);
  BOOST_CHECK_THROW(parser.parse({"route", "66"}, ParseMode::ONE_SHOT),
                    CommandParser::NoSuchCommandError);
  BOOST_CHECK_THROW(parser.parse({"route", "add"}, ParseMode::ONE_SHOT),
                    CommandDefinition::Error);
  BOOST_CHECK_THROW(parser.parse({"hidden"}, ParseMode::ONE_SHOT),
                    CommandParser::NoSuchCommandError);
}

BOOST_AUTO_TEST_SUITE_END() // TestCommandParser
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
