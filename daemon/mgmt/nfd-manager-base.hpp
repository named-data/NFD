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

#ifndef NFD_DAEMON_MGMT_NFD_MANAGER_BASE_HPP
#define NFD_DAEMON_MGMT_NFD_MANAGER_BASE_HPP

#include "common.hpp"
#include "command-validator.hpp"
#include "core/manager-base.hpp"

#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-control-response.hpp>
#include <ndn-cxx/management/nfd-control-parameters.hpp>

namespace nfd {

using ndn::mgmt::Dispatcher;
using ndn::nfd::ControlParameters;

/**
 * @brief a collection of common functions shared by all NFD managers,
 *        such as communicating with the dispatcher and command validator.
 */
class NfdManagerBase : public ManagerBase
{
public:
  NfdManagerBase(Dispatcher& dispatcher,
                 CommandValidator& validator,
                 const std::string& module);

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // command validation
  /**
   * @brief validate a request for ControlCommand.
   *
   * This is called by the dispatcher.
   *
   * @pre params != null
   * @pre typeid(*params) == typeid(ndn::nfd::ControlParameters)
   *
   * @param prefix the top prefix
   * @param interest a request for ControlCommand
   * @param params the parameters for ControlCommand
   * @param accept callback of successful validation, take the requester string as a argument
   * @param reject callback of failure in validation, take the action code as a argument
   */
  virtual void
  authorize(const Name& prefix, const Interest& interest,
            const ndn::mgmt::ControlParameters* params,
            ndn::mgmt::AcceptContinuation accept,
            ndn::mgmt::RejectContinuation reject) DECL_OVERRIDE;

private:
  CommandValidator& m_validator;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_NFD_MANAGER_BASE_HPP
