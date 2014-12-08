/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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
#include "core/logger.hpp"
#include "mgmt/app-face.hpp"

namespace nfd {

NFD_LOG_INIT("StrategyChoiceManager");

const Name StrategyChoiceManager::COMMAND_PREFIX = "/localhost/nfd/strategy-choice";

const size_t StrategyChoiceManager::COMMAND_UNSIGNED_NCOMPS =
  StrategyChoiceManager::COMMAND_PREFIX.size() +
  1 + // verb
  1;  // verb parameters

const size_t StrategyChoiceManager::COMMAND_SIGNED_NCOMPS =
  StrategyChoiceManager::COMMAND_UNSIGNED_NCOMPS +
  4; // (timestamp, nonce, signed info tlv, signature tlv)

const Name StrategyChoiceManager::LIST_DATASET_PREFIX("/localhost/nfd/strategy-choice/list");

StrategyChoiceManager::StrategyChoiceManager(StrategyChoice& strategyChoice,
                                             shared_ptr<InternalFace> face,
                                             ndn::KeyChain& keyChain)
  : ManagerBase(face, STRATEGY_CHOICE_PRIVILEGE, keyChain)
  , m_strategyChoice(strategyChoice)
  , m_listPublisher(strategyChoice, *m_face, LIST_DATASET_PREFIX, keyChain)
{
  face->setInterestFilter("/localhost/nfd/strategy-choice",
                          bind(&StrategyChoiceManager::onStrategyChoiceRequest, this, _2));
}

StrategyChoiceManager::~StrategyChoiceManager()
{

}

void
StrategyChoiceManager::onStrategyChoiceRequest(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (command == LIST_DATASET_PREFIX)
    {
      listStrategies(request);
      return;
    }
  else if (commandNComps <= COMMAND_PREFIX.size())
    {
      // command is too short to have a verb
      NFD_LOG_DEBUG("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  if (COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
      commandNComps < COMMAND_SIGNED_NCOMPS)
    {
      NFD_LOG_DEBUG("command result: unsigned verb: " << command);
      sendResponse(command, 401, "Signature required");

      return;
    }
  else if (commandNComps < COMMAND_SIGNED_NCOMPS ||
           !COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_DEBUG("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  validate(request,
           bind(&StrategyChoiceManager::onValidatedStrategyChoiceRequest, this, _1),
           bind(&ManagerBase::onCommandValidationFailed, this, _1, _2));
}

void
StrategyChoiceManager::listStrategies(const Interest& request)
{
  m_listPublisher.publish();
}

void
StrategyChoiceManager::onValidatedStrategyChoiceRequest(const shared_ptr<const Interest>& request)
{
  static const Name::Component VERB_SET("set");
  static const Name::Component VERB_UNSET("unset");

  const Name& command = request->getName();
  const Name::Component& parameterComponent = command[COMMAND_PREFIX.size() + 1];

  ControlParameters parameters;
  if (!extractParameters(parameterComponent, parameters))
    {
      sendResponse(command, 400, "Malformed command");
      return;
    }

  const Name::Component& verb = command.at(COMMAND_PREFIX.size());
  ControlResponse response;
  if (verb == VERB_SET)
    {
      setStrategy(parameters, response);
    }
  else if (verb == VERB_UNSET)
    {
      unsetStrategy(parameters, response);
    }
  else
    {
      NFD_LOG_DEBUG("command result: unsupported verb: " << verb);
      setResponse(response, 501, "Unsupported command");
    }

  sendResponse(command, response);
}

void
StrategyChoiceManager::setStrategy(ControlParameters& parameters,
                                   ControlResponse& response)
{
  ndn::nfd::StrategyChoiceSetCommand command;

  if (!validateParameters(command, parameters))
    {
      NFD_LOG_DEBUG("strategy-choice result: FAIL reason: malformed");
      setResponse(response, 400, "Malformed command");
      return;
    }

  const Name& prefix = parameters.getName();
  const Name& selectedStrategy = parameters.getStrategy();

  if (!m_strategyChoice.hasStrategy(selectedStrategy))
    {
      NFD_LOG_DEBUG("strategy-choice result: FAIL reason: unknown-strategy: "
                    << parameters.getStrategy());
      setResponse(response, 504, "Unsupported strategy");
      return;
    }

  if (m_strategyChoice.insert(prefix, selectedStrategy))
    {
      NFD_LOG_DEBUG("strategy-choice result: SUCCESS");
      auto currentStrategyChoice = m_strategyChoice.get(prefix);
      BOOST_ASSERT(currentStrategyChoice.first);
      parameters.setStrategy(currentStrategyChoice.second);
      setResponse(response, 200, "Success", parameters.wireEncode());
    }
  else
    {
      NFD_LOG_DEBUG("strategy-choice result: FAIL reason: not-installed");
      setResponse(response, 405, "Strategy not installed");
    }
}

void
StrategyChoiceManager::unsetStrategy(ControlParameters& parameters,
                                     ControlResponse& response)
{
  ndn::nfd::StrategyChoiceUnsetCommand command;

  if (!validateParameters(command, parameters))
    {
      static const Name ROOT_PREFIX;
      if (parameters.hasName() && parameters.getName() == ROOT_PREFIX)
        {
          NFD_LOG_DEBUG("strategy-choice result: FAIL reason: unset-root");
          setResponse(response, 403, "Cannot unset root prefix strategy");
        }
      else
        {
          NFD_LOG_DEBUG("strategy-choice result: FAIL reason: malformed");
          setResponse(response, 400, "Malformed command");
        }
      return;
    }

  m_strategyChoice.erase(parameters.getName());

  NFD_LOG_DEBUG("strategy-choice result: SUCCESS");
  setResponse(response, 200, "Success", parameters.wireEncode());
}



} // namespace nfd
