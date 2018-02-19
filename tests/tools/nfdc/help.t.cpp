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

#include "nfdc/help.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestHelp, BaseFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  CommandParser parser;
  ExecuteCommand dummyExecute = [] (ExecuteContext&) { BOOST_ERROR("should not be called"); };

  boost::test_tools::output_test_stream out;
  const std::string header("nfdc [-h|--help] [-V|--version] <command> [<args>]\n\n");
  const std::string trailer("\nSee 'nfdc help <command>' to read about a specific subcommand.\n");

  helpList(out, parser);
  BOOST_CHECK(out.is_equal(header + "All subcommands:\n"
                                    "  (none)\n"));

  parser.addCommand(CommandDefinition("status", "show"), dummyExecute);
  parser.addCommand(CommandDefinition("face", "list"), dummyExecute);
  parser.addCommand(CommandDefinition("route", "list"), dummyExecute);
  parser.addCommand(CommandDefinition("route", "add"), dummyExecute);
  parser.addCommand(CommandDefinition("batch", "command"), dummyExecute,
                    AVAILABLE_IN_BATCH | AVAILABLE_IN_HELP);

  helpList(out, parser);
  BOOST_CHECK(out.is_equal(header +
                           "All subcommands:\n"
                           "  status show     \n"
                           "  face list       \n"
                           "  route list      \n"
                           "  route add       \n" + trailer));

  helpList(out, parser, ParseMode::ONE_SHOT, "route");
  BOOST_CHECK(out.is_equal(header +
                           "Subcommands starting with route:\n"
                           "  route list      \n"
                           "  route add       \n" + trailer));

  helpList(out, parser, ParseMode::ONE_SHOT, "hello");
  BOOST_CHECK(out.is_equal(header +
                           "Subcommands starting with hello:\n"
                           "  (none)\n"));

  helpList(out, parser, ParseMode::BATCH);
  BOOST_CHECK(out.is_equal(header +
                           "All subcommands:\n"
                           "  status show     \n"
                           "  face list       \n"
                           "  route list      \n"
                           "  route add       \n"
                           "  batch command   \n" + trailer));

  BOOST_CHECK_EQUAL(help(out, parser, {}), 2);
  BOOST_CHECK(out.is_empty());

  BOOST_CHECK_EQUAL(help(out, parser, {"help"}), 0);
  BOOST_CHECK(out.is_equal(header +
                           "All subcommands:\n"
                           "  status show     \n"
                           "  face list       \n"
                           "  route list      \n"
                           "  route add       \n" + trailer));
}

BOOST_AUTO_TEST_SUITE_END() // TestCommandParser
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
