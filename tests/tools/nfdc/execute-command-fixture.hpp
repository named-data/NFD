/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#ifndef NFD_TESTS_TOOLS_NFDC_EXECUTE_COMMAND_FIXTURE_HPP
#define NFD_TESTS_TOOLS_NFDC_EXECUTE_COMMAND_FIXTURE_HPP

#include "mock-nfd-mgmt-fixture.hpp"
#include "nfdc/available-commands.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using boost::test_tools::output_test_stream;

/** \brief fixture to test command execution
 */
class ExecuteCommandFixture : public MockNfdMgmtFixture
{
protected:
  void
  execute(const std::string& cmd)
  {
    std::vector<std::string> args;
    boost::split(args, cmd, boost::is_any_of(" "));

    CommandParser parser;
    registerCommands(parser);

    std::string noun, verb;
    CommandArguments ca;
    ExecuteCommand execute;
    std::tie(noun, verb, ca, execute) = parser.parse(args, ParseMode::ONE_SHOT);

    Controller controller(face, m_keyChain);
    ExecuteContext ctx{noun, verb, ca, 0, out, err, face, m_keyChain, controller};
    execute(ctx);
    exitCode = ctx.exitCode;
  }

protected:
  output_test_stream out;
  output_test_stream err;
  int exitCode = -1;
};

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TESTS_TOOLS_NFDC_EXECUTE_COMMAND_FIXTURE_HPP
