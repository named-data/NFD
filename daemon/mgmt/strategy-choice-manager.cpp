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
  1;  // verb options

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

  ndn::nfd::FibManagementOptions options;
  if (!extractOptions(*request, options))
    {
      sendResponse(command, 400, "Malformed command");
      return;
    }

  const Name::Component& verb = command.get(COMMAND_PREFIX.size());
  ControlResponse response;
  if (verb == VERB_SET)
    {
      setStrategy(options, response);
    }
  else if (verb == VERB_UNSET)
    {
      unsetStrategy(options, response);
    }
  else
    {
      NFD_LOG_INFO("command result: unsupported verb: " << verb);
      setResponse(response, 501, "Unsupported command");
    }
  sendResponse(command, response);
}

bool
StrategyChoiceManager::extractOptions(const Interest& request,
                                      ndn::nfd::FibManagementOptions& extractedOptions)
{
  const Name& command = request.getName();
  const size_t optionCompIndex =
    COMMAND_PREFIX.size() + 1;

  try
    {
      Block rawOptions = request.getName()[optionCompIndex].blockFromValue();
      extractedOptions.wireDecode(rawOptions);
    }
  catch (const ndn::Tlv::Error& e)
    {
      NFD_LOG_INFO("Bad command option parse: " << command);
      return false;
    }

  NFD_LOG_DEBUG("Options parsed OK");
  return true;
}

void
StrategyChoiceManager::setStrategy(const ndn::nfd::FibManagementOptions& options,
                                   ControlResponse& response)
{
  const Name& prefix = options.getName();
  const Name& selectedStrategy = options.getStrategy();

  if (!m_strategyChoice.hasStrategy(selectedStrategy))
    {
      NFD_LOG_INFO("strategy-choice result: FAIL reason: unknown-strategy: "
                   << options.getStrategy());
      setResponse(response, 504, "Unsupported strategy");
      return;
    }

  if (m_strategyChoice.insert(prefix, selectedStrategy))
    {
      setResponse(response, 200, "Success", options.wireEncode());
    }
  else
    {
      setResponse(response, 405, "Strategy not installed");
    }
}

void
StrategyChoiceManager::unsetStrategy(const ndn::nfd::FibManagementOptions& options,
                                     ControlResponse& response)
{
  static const Name ROOT_PREFIX;

  const Name& prefix = options.getName();
  if (prefix == ROOT_PREFIX)
    {
      NFD_LOG_INFO("strategy-choice result: FAIL reason: unknown-prefix: "
                   << options.getName());
      setResponse(response, 403, "Cannot unset root prefix strategy");
      return;
    }

  m_strategyChoice.erase(prefix);
  setResponse(response, 200, "Success", options.wireEncode());
}



} // namespace nfd


