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

#include "strategy-choice-module.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
StrategyChoiceModule::registerCommands(CommandParser& parser)
{
  CommandDefinition defStrategyList("strategy", "list");
  defStrategyList
    .setTitle("print strategy choices");
  parser.addCommand(defStrategyList, &StrategyChoiceModule::list);
  parser.addAlias("strategy", "list", "");

  CommandDefinition defStrategyShow("strategy", "show");
  defStrategyShow
    .setTitle("show strategy choice of an entry")
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES);
  parser.addCommand(defStrategyShow, &StrategyChoiceModule::show);

  CommandDefinition defStrategySet("strategy", "set");
  defStrategySet
    .setTitle("set strategy choice for a name prefix")
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES)
    .addArg("strategy", ArgValueType::NAME, Required::YES, Positional::YES);
  parser.addCommand(defStrategySet, &StrategyChoiceModule::set);

  CommandDefinition defStrategyUnset("strategy", "unset");
  defStrategyUnset
    .setTitle("clear strategy choice at a name prefix")
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES);
  parser.addCommand(defStrategyUnset, &StrategyChoiceModule::unset);
}

void
StrategyChoiceModule::list(ExecuteContext& ctx)
{
  ctx.controller.fetch<ndn::nfd::StrategyChoiceDataset>(
    [&] (const std::vector<StrategyChoice>& dataset) {
      for (const StrategyChoice& entry : dataset) {
        formatItemText(ctx.out, entry);
        ctx.out << '\n';
      }
    },
    ctx.makeDatasetFailureHandler("strategy choice dataset"),
    ctx.makeCommandOptions());

  ctx.face.processEvents();
}

void
StrategyChoiceModule::show(ExecuteContext& ctx)
{
  auto prefix = ctx.args.get<Name>("prefix");

  ctx.controller.fetch<ndn::nfd::StrategyChoiceDataset>(
    [&] (const std::vector<StrategyChoice>& dataset) {
      StrategyChoice match; // longest prefix match
      for (const StrategyChoice& entry : dataset) {
        if (entry.getName().isPrefixOf(prefix) &&
            entry.getName().size() >= match.getName().size()) {
          match = entry;
        }
      }
      formatItemText(ctx.out, match, true);
    },
    ctx.makeDatasetFailureHandler("strategy choice dataset"),
    ctx.makeCommandOptions());

  ctx.face.processEvents();
}

void
StrategyChoiceModule::set(ExecuteContext& ctx)
{
  auto prefix = ctx.args.get<Name>("prefix");
  auto strategy = ctx.args.get<Name>("strategy");

  ctx.controller.start<ndn::nfd::StrategyChoiceSetCommand>(
    ControlParameters().setName(prefix).setStrategy(strategy),
    [&] (const ControlParameters& resp) {
      ctx.out << "strategy-set ";
      text::ItemAttributes ia;
      ctx.out << ia("prefix") << resp.getName()
              << ia("strategy") << resp.getStrategy() << '\n';
    },
    [&] (const ControlResponse& resp) {
      if (resp.getCode() == 404) {
        ctx.exitCode = 7;
        ctx.err << "Unknown strategy: " << strategy << '\n';
        ///\todo #3887 list available strategies
        return;
      }
      ctx.makeCommandFailureHandler("setting strategy")(resp); // invoke general error handler
    },
    ctx.makeCommandOptions());

  ctx.face.processEvents();
}

void
StrategyChoiceModule::unset(ExecuteContext& ctx)
{
  auto prefix = ctx.args.get<Name>("prefix");

  if (prefix.empty()) {
    ctx.exitCode = 2;
    ctx.err << "Unsetting default strategy is prohibited\n";
    return;
  }

  ctx.controller.start<ndn::nfd::StrategyChoiceUnsetCommand>(
    ControlParameters().setName(prefix),
    [&] (const ControlParameters& resp) {
      ctx.out << "strategy-unset ";
      text::ItemAttributes ia;
      ctx.out << ia("prefix") << resp.getName() << '\n';
    },
    ctx.makeCommandFailureHandler("unsetting strategy"),
    ctx.makeCommandOptions());

  ctx.face.processEvents();
}

void
StrategyChoiceModule::fetchStatus(Controller& controller,
                                  const std::function<void()>& onSuccess,
                                  const Controller::DatasetFailCallback& onFailure,
                                  const CommandOptions& options)
{
  controller.fetch<ndn::nfd::StrategyChoiceDataset>(
    [this, onSuccess] (const std::vector<StrategyChoice>& result) {
      m_status = result;
      onSuccess();
    },
    onFailure, options);
}

void
StrategyChoiceModule::formatStatusXml(std::ostream& os) const
{
  os << "<strategyChoices>";
  for (const StrategyChoice& item : m_status) {
    this->formatItemXml(os, item);
  }
  os << "</strategyChoices>";
}

void
StrategyChoiceModule::formatItemXml(std::ostream& os, const StrategyChoice& item) const
{
  os << "<strategyChoice>";
  os << "<namespace>" << xml::Text{item.getName().toUri()} << "</namespace>";
  os << "<strategy><name>" << xml::Text{item.getStrategy().toUri()} << "</name></strategy>";
  os << "</strategyChoice>";
}

void
StrategyChoiceModule::formatStatusText(std::ostream& os) const
{
  os << "Strategy choices:\n";
  for (const StrategyChoice& item : m_status) {
    os << "  ";
    formatItemText(os, item);
    os << '\n';
  }
}

void
StrategyChoiceModule::formatItemText(std::ostream& os, const StrategyChoice& item, bool wantMultiLine)
{
  text::ItemAttributes ia(wantMultiLine, 8);
  os << ia("prefix") << item.getName()
     << ia("strategy") << item.getStrategy()
     << ia.end();
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
