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

#include "strategy-choice-manager.hpp"
#include "table/strategy-choice.hpp"
#include <ndn-cxx/mgmt/nfd/strategy-choice.hpp>

namespace nfd {

NFD_LOG_INIT("StrategyChoiceManager");

StrategyChoiceManager::StrategyChoiceManager(StrategyChoice& strategyChoice,
                                             Dispatcher& dispatcher,
                                             CommandAuthenticator& authenticator)
  : NfdManagerBase(dispatcher, authenticator, "strategy-choice")
  , m_table(strategyChoice)
{
  registerCommandHandler<ndn::nfd::StrategyChoiceSetCommand>("set",
    bind(&StrategyChoiceManager::setStrategy, this, _2, _3, _4, _5));
  registerCommandHandler<ndn::nfd::StrategyChoiceUnsetCommand>("unset",
    bind(&StrategyChoiceManager::unsetStrategy, this, _2, _3, _4, _5));

  registerStatusDatasetHandler("list",
    bind(&StrategyChoiceManager::listChoices, this, _1, _2, _3));
}

void
StrategyChoiceManager::setStrategy(const Name& topPrefix, const Interest& interest,
                                   ControlParameters parameters,
                                   const ndn::mgmt::CommandContinuation& done)
{
  const Name& prefix = parameters.getName();
  const Name& selectedStrategy = parameters.getStrategy();

  if (!m_table.hasStrategy(selectedStrategy)) {
    NFD_LOG_DEBUG("strategy-choice result: FAIL reason: unknown-strategy: "
                  << parameters.getStrategy());
    return done(ControlResponse(504, "Unsupported strategy"));
  }

  if (m_table.insert(prefix, selectedStrategy)) {
    NFD_LOG_DEBUG("strategy-choice result: SUCCESS");
    auto currentStrategyChoice = m_table.get(prefix);
    BOOST_ASSERT(currentStrategyChoice.first);
    parameters.setStrategy(currentStrategyChoice.second);
    return done(ControlResponse(200, "OK").setBody(parameters.wireEncode()));
  }
  else {
    NFD_LOG_DEBUG("strategy-choice result: FAIL reason: not-installed");
    return done(ControlResponse(405, "Strategy not installed"));
  }
}

void
StrategyChoiceManager::unsetStrategy(const Name& topPrefix, const Interest& interest,
                                     ControlParameters parameters,
                                     const ndn::mgmt::CommandContinuation& done)
{
  m_table.erase(parameters.getName());

  NFD_LOG_DEBUG("strategy-choice result: SUCCESS");
  done(ControlResponse(200, "OK").setBody(parameters.wireEncode()));
}

void
StrategyChoiceManager::listChoices(const Name& topPrefix, const Interest& interest,
                                   ndn::mgmt::StatusDatasetContext& context)
{
  for (auto&& i : m_table) {
    ndn::nfd::StrategyChoice entry;
    entry.setName(i.getPrefix()).setStrategy(i.getStrategyName());
    context.append(entry.wireEncode());
  }
  context.end();
}

} // namespace nfd
