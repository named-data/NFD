/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "strategy-choice-manager.hpp"
#include "table/strategy-choice.hpp"
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

StrategyChoiceManager::StrategyChoiceManager(StrategyChoice& strategyChoice,
                                             shared_ptr<InternalFace> face)
  : ManagerBase(face, STRATEGY_CHOICE_PRIVILEGE)
  , m_strategyChoice(strategyChoice)
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

  if (COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
      commandNComps < COMMAND_SIGNED_NCOMPS)
    {
      NFD_LOG_INFO("command result: unsigned verb: " << command);
      sendResponse(command, 401, "Signature required");

      return;
    }
  else if (commandNComps < COMMAND_SIGNED_NCOMPS ||
           !COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_INFO("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  validate(request,
           bind(&StrategyChoiceManager::onValidatedStrategyChoiceRequest, this, _1),
           bind(&ManagerBase::onCommandValidationFailed, this, _1, _2));
}

void
StrategyChoiceManager::onValidatedStrategyChoiceRequest(const shared_ptr<const Interest>& request)
{
  static const Name::Component VERB_SET("set");
  static const Name::Component VERB_UNSET("unset");

  const Name& command = request->getName();
  const Name::Component& parameterComponent = command[COMMAND_PREFIX.size() + 1];

  ControlParameters parameters;
  if (!extractParameters(parameterComponent, parameters) || !parameters.hasName())
    {
      sendResponse(command, 400, "Malformed command");
      return;
    }

  const Name::Component& verb = command[COMMAND_PREFIX.size()];
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
      NFD_LOG_INFO("command result: unsupported verb: " << verb);
      setResponse(response, 501, "Unsupported command");
    }
  sendResponse(command, response);
}

void
StrategyChoiceManager::setStrategy(const ControlParameters& parameters,
                                   ControlResponse& response)
{
  if (!parameters.hasStrategy())
    {
      setResponse(response, 400, "Malformed command");
      return;
    }

  const Name& prefix = parameters.getName();
  const Name& selectedStrategy = parameters.getStrategy();

  if (!m_strategyChoice.hasStrategy(selectedStrategy))
    {
      NFD_LOG_INFO("strategy-choice result: FAIL reason: unknown-strategy: "
                   << parameters.getStrategy());
      setResponse(response, 504, "Unsupported strategy");
      return;
    }

  if (m_strategyChoice.insert(prefix, selectedStrategy))
    {
      setResponse(response, 200, "Success", parameters.wireEncode());
    }
  else
    {
      setResponse(response, 405, "Strategy not installed");
    }
}

void
StrategyChoiceManager::unsetStrategy(const ControlParameters& parameters,
                                     ControlResponse& response)
{
  static const Name ROOT_PREFIX;

  const Name& prefix = parameters.getName();
  if (prefix == ROOT_PREFIX)
    {
      NFD_LOG_INFO("strategy-choice result: FAIL reason: unknown-prefix: "
                   << parameters.getName());
      setResponse(response, 403, "Cannot unset root prefix strategy");
      return;
    }

  m_strategyChoice.erase(prefix);
  setResponse(response, 200, "Success", parameters.wireEncode());
}



} // namespace nfd


