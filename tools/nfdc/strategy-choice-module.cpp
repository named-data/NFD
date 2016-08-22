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

#include "strategy-choice-module.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
StrategyChoiceModule::fetchStatus(Controller& controller,
                                  const function<void()>& onSuccess,
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
    this->formatItemText(os, item);
  }
}

void
StrategyChoiceModule::formatItemText(std::ostream& os, const StrategyChoice& item) const
{
  os << "  " << item.getName()
     << " strategy=" << item.getStrategy()
     << "\n";
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
