/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_STRATEGY_CHOICE_MANAGER_HPP
#define NFD_MGMT_STRATEGY_CHOICE_MANAGER_HPP

#include "mgmt/manager-base.hpp"

#include <ndn-cpp-dev/management/nfd-control-parameters.hpp>

namespace nfd {

const std::string STRATEGY_CHOICE_PRIVILEGE = "strategy-choice";

class StrategyChoice;

class StrategyChoiceManager : public ManagerBase
{
public:
  StrategyChoiceManager(StrategyChoice& strategyChoice,
                        shared_ptr<InternalFace> face);

  virtual
  ~StrategyChoiceManager();

  void
  onStrategyChoiceRequest(const Interest& request);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  onValidatedStrategyChoiceRequest(const shared_ptr<const Interest>& request);

  void
  setStrategy(const ControlParameters& parameters,
              ControlResponse& response);

  void
  unsetStrategy(const ControlParameters& parameters,
                ControlResponse& response);
private:

  StrategyChoice& m_strategyChoice;

  static const Name COMMAND_PREFIX; // /localhost/nfd/strategy-choice

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nfd/strategy-choice + verb + parameters) = 5
  static const size_t COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // (see UNSIGNED_NCOMPS), 9 with signed Interest support.
  static const size_t COMMAND_SIGNED_NCOMPS;

};

} // namespace nfd

#endif // NFD_MGMT_STRATEGY_CHOICE_MANAGER_HPP

