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

#include "status.hpp"
#include "forwarder-general-module.hpp"
#include "channel-module.hpp"
#include "face-module.hpp"
#include "fib-module.hpp"
#include "rib-module.hpp"
#include "cs-module.hpp"
#include "strategy-choice-module.hpp"

#include <ndn-cxx/security/validator-null.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

void
reportStatus(ExecuteContext& ctx, const StatusReportOptions& options)
{
  StatusReport report;

  if (options.wantForwarderGeneral) {
    report.sections.push_back(make_unique<ForwarderGeneralModule>());
  }

  if (options.wantChannels) {
    report.sections.push_back(make_unique<ChannelModule>());
  }

  if (options.wantFaces) {
    report.sections.push_back(make_unique<FaceModule>());
  }

  if (options.wantFib) {
    report.sections.push_back(make_unique<FibModule>());
  }

  if (options.wantRib) {
    report.sections.push_back(make_unique<RibModule>());
  }

  if (options.wantCs) {
    report.sections.push_back(make_unique<CsModule>());
  }

  if (options.wantStrategyChoice) {
    report.sections.push_back(make_unique<StrategyChoiceModule>());
  }

  uint32_t code = report.collect(ctx.face, ctx.keyChain,
                                 ndn::security::v2::getAcceptAllValidator(),
                                 CommandOptions());
  if (code != 0) {
    ctx.exitCode = 1;
    // Give a simple error code for end user.
    // Technical support personnel:
    // 1. get the exact command from end user
    // 2. code div 1000000 is zero-based section index
    // 3. code mod 1000000 is a Controller.fetch error code
    ctx.err << "Error while collecting status report (" << code << ").\n";
  }

  switch (options.output) {
    case ReportFormat::XML:
      report.formatXml(ctx.out);
      break;
    case ReportFormat::TEXT:
      report.formatText(ctx.out);
      break;
  }
}

/** \brief single-section status command
 */
static void
reportStatusSingleSection(ExecuteContext& ctx, bool StatusReportOptions::*wantSection)
{
  StatusReportOptions options;
  options.*wantSection = true;
  reportStatus(ctx, options);
}

/** \brief the 'status report' command
 */
static void
reportStatusComprehensive(ExecuteContext& ctx)
{
  StatusReportOptions options;
  options.output = ctx.args.get<ReportFormat>("format", ReportFormat::TEXT);
  options.wantForwarderGeneral = options.wantChannels = options.wantFaces = options.wantFib =
    options.wantRib = options.wantCs = options.wantStrategyChoice = true;
  reportStatus(ctx, options);
}

void
registerStatusCommands(CommandParser& parser)
{
  CommandDefinition defStatusReport("status", "report");
  defStatusReport
    .setTitle("print full status report")
    .addArg("format", ArgValueType::REPORT_FORMAT, Required::NO, Positional::YES);
  parser.addCommand(defStatusReport, &reportStatusComprehensive);

  CommandDefinition defStatusShow("status", "show");
  defStatusShow
    .setTitle("print general status");
  parser.addCommand(defStatusShow, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantForwarderGeneral));
  parser.addAlias("status", "show", "");

  CommandDefinition defChannelList("channel", "list");
  defChannelList
    .setTitle("print channel list");
  parser.addCommand(defChannelList, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantChannels));
  parser.addAlias("channel", "list", "");

  CommandDefinition defFibList("fib", "list");
  defFibList
    .setTitle("print FIB entries");
  parser.addCommand(defFibList, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantFib));
  parser.addAlias("fib", "list", "");

  CommandDefinition defCsInfo("cs", "info");
  defCsInfo
    .setTitle("print CS information");
  parser.addCommand(defCsInfo, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantCs));
  parser.addAlias("cs", "info", "");
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
