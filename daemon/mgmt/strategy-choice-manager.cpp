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

#include "strategy-choice-manager.hpp"
#include "table/strategy-choice.hpp"
#include <ndn-cxx/mgmt/nfd/strategy-choice.hpp>
#include <boost/lexical_cast.hpp>

namespace nfd {

NFD_LOG_INIT("StrategyChoiceManager");

StrategyChoiceManager::StrategyChoiceManager(StrategyChoice& strategyChoice,
                                             Dispatcher& dispatcher,
                                             CommandAuthenticator& authenticator)
  : NfdManagerBase(dispatcher, authenticator, "strategy-choice")
  , m_table(strategyChoice)
{
  registerCommandHandler<ndn::nfd::StrategyChoiceSetCommand>("set",
    bind(&StrategyChoiceManager::setStrategy, this, _4, _5));
  registerCommandHandler<ndn::nfd::StrategyChoiceUnsetCommand>("unset",
    bind(&StrategyChoiceManager::unsetStrategy, this, _4, _5));

  registerStatusDatasetHandler("list",
    bind(&StrategyChoiceManager::listChoices, this, _3));
}

void
StrategyChoiceManager::setStrategy(ControlParameters parameters,
                                   const ndn::mgmt::CommandContinuation& done)
{
  const Name& prefix = parameters.getName();
  const Name& strategy = parameters.getStrategy();

  StrategyChoice::InsertResult res = m_table.insert(prefix, strategy);
  if (!res) {
    NFD_LOG_DEBUG("strategy-choice/set(" << prefix << "," << strategy << "): cannot-create " << res);
    return done(ControlResponse(res.getStatusCode(), boost::lexical_cast<std::string>(res)));
  }

  NFD_LOG_DEBUG("strategy-choice/set(" << prefix << "," << strategy << "): OK");
  bool hasEntry = false;
  Name instanceName;
  std::tie(hasEntry, instanceName) = m_table.get(prefix);
  BOOST_ASSERT_MSG(hasEntry, "StrategyChoice entry must exist after StrategyChoice::insert");
  parameters.setStrategy(instanceName);
  return done(ControlResponse(200, "OK").setBody(parameters.wireEncode()));
}

void
StrategyChoiceManager::unsetStrategy(ControlParameters parameters,
                                     const ndn::mgmt::CommandContinuation& done)
{
  const Name& prefix = parameters.getName();
  // no need to test for ndn:/ , parameter validation takes care of that

  m_table.erase(parameters.getName());

  NFD_LOG_DEBUG("strategy-choice/unset(" << prefix << "): OK");
  done(ControlResponse(200, "OK").setBody(parameters.wireEncode()));
}

void
StrategyChoiceManager::listChoices(ndn::mgmt::StatusDatasetContext& context)
{
  for (const auto& i : m_table) {
    ndn::nfd::StrategyChoice entry;
    entry.setName(i.getPrefix())
         .setStrategy(i.getStrategyInstanceName());
    context.append(entry.wireEncode());
  }
  context.end();
}

} // namespace nfd
