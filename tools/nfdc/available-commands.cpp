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

#include "available-commands.hpp"
#include "face-module.hpp"
#include "help.hpp"
#include "legacy-nfdc.hpp"
#include "legacy-status.hpp"
#include "rib-module.hpp"
#include "status.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
registerCommands(CommandParser& parser)
{
  registerHelpCommand(parser);
  registerStatusCommands(parser);
  FaceModule::registerCommands(parser);
  RibModule::registerCommands(parser);

  registerLegacyStatusCommand(parser);

  struct LegacyNfdcCommandDefinition
  {
    std::string subcommand;
    std::string title;
    std::string replacementCommand; ///< replacement for deprecated legacy subcommand
  };
  const std::vector<LegacyNfdcCommandDefinition> legacyNfdcSubcommands{
    {"register", "register a prefix", "route add"},
    {"unregister", "unregister a prefix", "route remove"},
    {"create", "create a face", "face create"},
    {"destroy", "destroy a face", "face destroy"},
    {"set-strategy", "set strategy choice on namespace", ""},
    {"unset-strategy", "unset strategy choice on namespace", ""},
    {"add-nexthop", "add FIB nexthop", "route add"},
    {"remove-nexthop", "remove FIB nexthop", "route remove"}
  };
  for (const LegacyNfdcCommandDefinition& lncd : legacyNfdcSubcommands) {
    CommandDefinition def(lncd.subcommand, "");
    def.setTitle(lncd.title);
    def.addArg("args", ArgValueType::ANY, Required::NO, Positional::YES);
    auto modes = AVAILABLE_IN_ONE_SHOT | AVAILABLE_IN_HELP;
    if (!lncd.replacementCommand.empty()) {
      modes = modes & ~AVAILABLE_IN_HELP;
    }
    parser.addCommand(def, bind(&legacyNfdcMain, _1, lncd.replacementCommand), modes);
  }
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
