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

#include "help.hpp"
#include "format-helpers.hpp"
#include <ndn-cxx/util/logger.hpp>
#include <unistd.h>

namespace nfd {
namespace tools {
namespace nfdc {

NDN_LOG_INIT(nfdc.Help);

const int LIST_COMMAND_NAME_COLUMN_WIDTH = 16;

void
helpList(std::ostream& os, const CommandParser& parser, ParseMode mode, const std::string& noun)
{
  os << "nfdc [-h] [-V] <command> [<args>]\n\n";
  if (noun.empty()) {
    os << "All subcommands:\n";
  }
  else {
    os << "Subcommands starting with " << noun << ":\n";
  }

  std::vector<const CommandDefinition*> commands = parser.listCommands(noun, mode);
  if (commands.empty()) {
    os << "  (none)\n";
    return;
  }

  for (auto def : commands) {
    os << "  " << def->getNoun() << ' ' << def->getVerb() << ' '
       << text::Spaces{static_cast<int>(LIST_COMMAND_NAME_COLUMN_WIDTH -
                       def->getNoun().size() - def->getVerb().size() - 2)}
       << def->getTitle() << '\n';
  }

  os << "\nSee 'nfdc help <command>' to read about a specific subcommand.\n";
}

static void
helpSingle(const std::string& noun, const std::string& verb)
{
  std::string manpage = "nfdc-" + noun;

  execlp("man", "man", manpage.data(), nullptr);
  NDN_LOG_FATAL("Error opening man page for " << manpage);
}

void
help(ExecuteContext& ctx, const CommandParser& parser)
{
  std::string noun = ctx.args.get<std::string>("noun", "");
  std::string verb = ctx.args.get<std::string>("verb", "");

  if (noun.empty()) {
    helpList(ctx.out, parser, ParseMode::ONE_SHOT, noun);
  }
  else {
    helpSingle(noun, verb); // should not return
    ctx.exitCode = 1;
  }
}

void
registerHelpCommand(CommandParser& parser)
{
  CommandDefinition defHelp("help", "");
  defHelp
    .setTitle("display help information")
    .addArg("noun", ArgValueType::STRING, Required::NO, Positional::YES)
    .addArg("verb", ArgValueType::STRING, Required::NO, Positional::YES);
  parser.addCommand(defHelp, bind(&help, _1, cref(parser)));
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
