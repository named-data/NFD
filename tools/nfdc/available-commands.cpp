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

#include "available-commands.hpp"
#include "status-report.hpp"
#include "status-main.hpp"
#include "legacy-nfdc.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

static void
statusReport(const CommandArguments& ca)
{
  int res = 1;
  ReportFormat fmt = ca.get<ReportFormat>("format", ReportFormat::TEXT);
  switch (fmt) {
    case ReportFormat::XML:
      res = statusMain(std::vector<std::string>{"-x"});
      break;
    case ReportFormat::TEXT:
      res = statusMain(std::vector<std::string>{});
      break;
  }
  exit(res);
}

static void
legacyNfdStatus(const CommandArguments& ca)
{
  std::vector<std::string> args = ca.get<std::vector<std::string>>("args");
  int res = statusMain(args);
  exit(res);
}

static void
legacyNfdc(const std::string& subcommand, const CommandArguments& ca)
{
  std::vector<std::string> args = ca.get<std::vector<std::string>>("args");
  int res = legacyNfdcMain(subcommand, args);
  exit(res);
}

void
registerCommands(CommandParser& parser)
{
  CommandDefinition defStatusReport("status", "report");
  defStatusReport
    .addArg("format", ArgValueType::REPORT_FORMAT, Required::NO, Positional::YES);
  parser.addCommand(defStatusReport, &statusReport);

  CommandDefinition defLegacyNfdStatus("legacy-nfd-status", "");
  defLegacyNfdStatus
    .addArg("args", ArgValueType::ANY, Required::NO, Positional::YES);
  parser.addCommand(defLegacyNfdStatus, &legacyNfdStatus);

  const std::vector<std::string> legacyNfdcSubcommands{
    "register",
    "unregister",
    "create",
    "destroy",
    "set-strategy",
    "unset-strategy",
    "add-nexthop",
    "remove-nexthop"
  };
  for (const std::string& subcommand : legacyNfdcSubcommands) {
    CommandDefinition def(subcommand, "");
    def.addArg("args", ArgValueType::ANY, Required::NO, Positional::YES);
    parser.addCommand(def, bind(&legacyNfdc, subcommand, _1));
  }
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
