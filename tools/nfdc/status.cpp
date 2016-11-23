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

#include "status.hpp"
#include "forwarder-general-module.hpp"
#include "channel-module.hpp"
#include "face-module.hpp"
#include "fib-module.hpp"
#include "rib-module.hpp"
#include "strategy-choice-module.hpp"

#include <ndn-cxx/security/validator-null.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

int
reportStatus(ExecuteContext& ctx, const StatusReportOptions& options)
{
  unique_ptr<Validator> validator = make_unique<ndn::ValidatorNull>();
  CommandOptions ctrlOptions;

  StatusReport report;

  if (options.wantForwarderGeneral) {
    auto nfdIdCollector = make_unique<NfdIdCollector>(std::move(validator));
    auto forwarderGeneralModule = make_unique<ForwarderGeneralModule>();
    forwarderGeneralModule->setNfdIdCollector(*nfdIdCollector);
    report.sections.push_back(std::move(forwarderGeneralModule));
    validator = std::move(nfdIdCollector);
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

  if (options.wantStrategyChoice) {
    report.sections.push_back(make_unique<StrategyChoiceModule>());
  }

  uint32_t code = report.collect(ctx.face, ctx.keyChain, *validator, ctrlOptions);
  if (code != 0) {
    // Give a simple error code for end user.
    // Technical support personnel:
    // 1. get the exact command from end user
    // 2. code div 1000000 is zero-based section index
    // 3. code mod 1000000 is a Controller.fetch error code
    std::cerr << "Error while collecting status report (" << code << ").\n";
    return 1;
  }

  switch (options.output) {
    case ReportFormat::XML:
      report.formatXml(std::cout);
      break;
    case ReportFormat::TEXT:
      report.formatText(std::cout);
      break;
  }
  return 0;
}

/** \brief single-section status command
 */
static int
reportStatusSingleSection(ExecuteContext& ctx, bool StatusReportOptions::*wantSection)
{
  StatusReportOptions options;
  options.*wantSection = true;
  return reportStatus(ctx, options);
}

/** \brief the 'status report' command
 */
static int
reportStatusComprehensive(ExecuteContext& ctx)
{
  StatusReportOptions options;
  options.output = ctx.args.get<ReportFormat>("format", ReportFormat::TEXT);
  options.wantForwarderGeneral = options.wantChannels = options.wantFaces =
    options.wantFib = options.wantRib = options.wantStrategyChoice = true;
  return reportStatus(ctx, options);
}

void
registerStatusCommands(CommandParser& parser)
{
  CommandDefinition defStatusReport("status", "report");
  defStatusReport
    .setTitle("print NFD status report")
    .addArg("format", ArgValueType::REPORT_FORMAT, Required::NO, Positional::YES);
  parser.addCommand(defStatusReport, &reportStatusComprehensive);

  CommandDefinition defStatusShow("status", "show");
  defStatusShow
    .setTitle("print general status");
  parser.addCommand(defStatusShow, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantForwarderGeneral));
  parser.addAlias("status", "show", "list");

  CommandDefinition defFaceList("face", "list");
  defFaceList
    .setTitle("print face list");
  parser.addCommand(defFaceList, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantFaces));

  CommandDefinition defChannelList("channel", "list");
  defChannelList
    .setTitle("print channel list");
  parser.addCommand(defChannelList, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantChannels));

  CommandDefinition defStrategyList("strategy", "list");
  defStrategyList
    .setTitle("print strategy choices");
  parser.addCommand(defStrategyList, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantStrategyChoice));

  CommandDefinition defFibList("fib", "list");
  defFibList
    .setTitle("print FIB entries");
  parser.addCommand(defFibList, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantFib));

  CommandDefinition defRouteList("route", "list");
  defRouteList
    .setTitle("print RIB entries");
  parser.addCommand(defRouteList, bind(&reportStatusSingleSection, _1, &StatusReportOptions::wantRib));
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
