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

#ifndef NFD_DAEMON_MGMT_STRATEGY_CHOICE_MANAGER_HPP
#define NFD_DAEMON_MGMT_STRATEGY_CHOICE_MANAGER_HPP

#include "mgmt/manager-base.hpp"
#include "mgmt/strategy-choice-publisher.hpp"

#include <ndn-cxx/management/nfd-control-parameters.hpp>

namespace nfd {

const std::string STRATEGY_CHOICE_PRIVILEGE = "strategy-choice";

class StrategyChoice;

class StrategyChoiceManager : public ManagerBase
{
public:
  StrategyChoiceManager(StrategyChoice& strategyChoice,
                        shared_ptr<InternalFace> face,
                        ndn::KeyChain& keyChain);

  virtual
  ~StrategyChoiceManager();

  void
  onStrategyChoiceRequest(const Interest& request);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:

  void
  listStrategies(const Interest& request);

  void
  onValidatedStrategyChoiceRequest(const shared_ptr<const Interest>& request);

  void
  setStrategy(ControlParameters& parameters,
              ControlResponse& response);

  void
  unsetStrategy(ControlParameters& parameters,
                ControlResponse& response);

private:

  StrategyChoice& m_strategyChoice;

  StrategyChoicePublisher m_listPublisher;

  static const Name COMMAND_PREFIX; // /localhost/nfd/strategy-choice

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nfd/strategy-choice + verb + parameters) = 5
  static const size_t COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // (see UNSIGNED_NCOMPS), 9 with signed Interest support.
  static const size_t COMMAND_SIGNED_NCOMPS;

  static const Name LIST_DATASET_PREFIX; // /localhost/nfd/strategy-choice/list

};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_STRATEGY_CHOICE_MANAGER_HPP
