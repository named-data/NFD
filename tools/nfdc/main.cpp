/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
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

#include "available-commands.hpp"
#include "core/version.hpp"
#include "help.hpp"

#include <boost/tokenizer.hpp>
#include <fstream>
#include <iostream>

namespace nfd {
namespace tools {
namespace nfdc {

static int
main(int argc, char** argv)
{
  std::vector<std::string> args(argv + 1, argv + argc);

  CommandParser parser;
  registerCommands(parser);

  if (args.empty()) {
    helpList(std::cout, parser);
    return 0;
  }

  if (args[0] == "-V" || args[0] == "--version") {
    std::cout << NFD_VERSION_BUILD_STRING << std::endl;
    return 0;
  }

  struct Command
  {
    std::string noun, verb;
    CommandArguments ca;
    ExecuteCommand execute;
  };

  auto processLine = [&parser] (const std::vector<std::string>& line) -> Command {
    try {
      Command cmd;
      std::tie(cmd.noun, cmd.verb, cmd.ca, cmd.execute) = parser.parse(line, ParseMode::ONE_SHOT);
      return cmd;
    }
    catch (const std::invalid_argument& e) {
      int ret = help(std::cout, parser, line);
      if (ret == 2)
        std::cerr << e.what() << std::endl;
      return {"", "", {}, nullptr};
    }
  };

  std::list<Command> commands;

  if (args[0] == "-f" || args[0] == "--batch") {
    if (args.size() != 2) {
      std::cerr << "ERROR: Invalid command line arguments: " << args[0] << " should follow with batch-file."
                << " Use -h for more detail." << std::endl;
      return 2;
    }

    auto processIstream = [&commands,&processLine] (std::istream& is, const std::string& inputFile) {
      std::string line;
      size_t lineCounter = 0;
      while (std::getline(is, line)) {
        ++lineCounter;

        auto hasEscapeSlash = [] (const std::string& str) {
          auto count = std::count(str.rbegin(), str.rend(), '\\');
          return (count % 2) == 1;
        };
        while (!line.empty() && hasEscapeSlash(line)) {
          std::string extraLine;
          const auto& hasMore = std::getline(is, extraLine);
          ++lineCounter;
          line = line.substr(0, line.size() - 1) + extraLine;
          if (!hasMore) {
            break;
          }
        }
        boost::tokenizer<boost::escaped_list_separator<char>> tokenizer(
          line,
          boost::escaped_list_separator<char>("\\", " \t", "\"'"));

        auto firstNonEmptyToken = tokenizer.begin();
        while (firstNonEmptyToken != tokenizer.end() && firstNonEmptyToken->empty()) {
          ++firstNonEmptyToken;
        }

        // Ignore empty lines (or lines with just spaces) and lines that start with #
        // Non empty lines with trailing comment are not allowed and may trigger syntax error
        if (firstNonEmptyToken == tokenizer.end() || (*firstNonEmptyToken)[0] == '#') {
          continue;
        }

        std::vector<std::string> lineArgs;
        std::copy_if(firstNonEmptyToken, tokenizer.end(),
                     std::back_inserter<std::vector<std::string>>(lineArgs),
                     [] (const std::string& t) { return !t.empty(); });

        auto cmd = processLine(lineArgs);
        if (cmd.noun.empty()) {
          std::cerr << "  >> Syntax error on line " << lineCounter << " of the batch in "
                    << inputFile << std::endl;
          return 2; // not exactly correct, but should be indication of an error, which already shown
        }
        commands.push_back(std::move(cmd));
      }
      return 0;
    };

    if (args[1] == "-") {
      auto retval = processIstream(std::cin, "standard input");
      if (retval != 0) {
        return retval;
      }
    }
    else {
      std::ifstream iff(args[1]);
      if (!iff) {
        std::cerr << "ERROR: Could not open `" << args[1] << "` batch file "
                  << "(" << strerror(errno) << ")" << std::endl;
        return 2;
      }
      auto retval = processIstream(iff, args[1]);
      if (retval != 0) {
        return retval;
      }
    }
  }
  else {
    commands.push_back(processLine(args));
  }

  try {
    Face face;
    KeyChain keyChain;
    Controller controller(face, keyChain);
    size_t commandCounter = 0;
    for (auto& command : commands) {
      ++commandCounter;
      ExecuteContext ctx{command.noun, command.verb, command.ca, 0,
                         std::cout, std::cerr, face, keyChain, controller};
      command.execute(ctx);

      if (ctx.exitCode != 0) {
        if (commands.size() > 1) {
          std::cerr << "  >> Failed to execute command on line " << commandCounter
                    << " of the batch file " << args[1] << std::endl;
          std::cerr << "  Note that nfdc has executed all commands on previous lines and "
                    << "stopped processing at this line" << std::endl;
        }

        return ctx.exitCode;
      }
    }
    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}

} // namespace nfdc
} // namespace tools
} // namespace nfd

int
main(int argc, char** argv)
{
  return nfd::tools::nfdc::main(argc, argv);
}
