/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_DAEMON_MGMT_MANAGER_BASE_HPP
#define NFD_DAEMON_MGMT_MANAGER_BASE_HPP

#include "command-authenticator.hpp"

#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>
#include <ndn-cxx/mgmt/nfd/control-response.hpp>

namespace nfd {

using ndn::mgmt::Dispatcher;
using ndn::nfd::ControlCommand;
using ndn::nfd::ControlParameters;
using ndn::nfd::ControlResponse;

/**
 * @brief A collection of common functions shared by all NFD managers,
 *        such as communicating with the dispatcher and command validator.
 */
class ManagerBase : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  virtual
  ~ManagerBase();

  const std::string&
  getModule() const
  {
    return m_module;
  }

protected:
  /**
   * @warning if you use this constructor, you MUST override makeAuthorization()
   */
  ManagerBase(const std::string& module, Dispatcher& dispatcher);

  ManagerBase(const std::string& module, Dispatcher& dispatcher,
              CommandAuthenticator& authenticator);

PUBLIC_WITH_TESTS_ELSE_PROTECTED: // registrations to the dispatcher
  // difference from mgmt::ControlCommand: accepts nfd::ControlParameters
  using ControlCommandHandler = std::function<void(const ControlCommand& command,
                                                   const Name& prefix, const Interest& interest,
                                                   const ControlParameters& parameters,
                                                   const ndn::mgmt::CommandContinuation done)>;

  template<typename Command>
  void
  registerCommandHandler(const std::string& verb,
                         const ControlCommandHandler& handler);

  void
  registerStatusDatasetHandler(const std::string& verb,
                               const ndn::mgmt::StatusDatasetHandler& handler);

  ndn::mgmt::PostNotification
  registerNotificationStream(const std::string& verb);

PUBLIC_WITH_TESTS_ELSE_PROTECTED: // helpers
  /**
   * @brief Extracts the requester from a ControlCommand request.
   *
   * This is called after the signature has been validated.
   *
   * @param interest a request for ControlCommand
   * @param accept callback of successful validation, takes the requester string as a argument
   */
  void
  extractRequester(const Interest& interest, ndn::mgmt::AcceptContinuation accept);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /**
   * @brief Returns an authorization function for a specific management module and verb.
   */
  virtual ndn::mgmt::Authorization
  makeAuthorization(const std::string& verb);

  /**
   * @brief Validates the @p parameters for a given @p command.
   *
   * @param parameters the original ControlParameters
   * @return whether the original ControlParameters can be validated
   */
  static bool
  validateParameters(const ControlCommand& command,
                     const ndn::mgmt::ControlParameters& parameters);

  /**
   * @brief Handles a control command.
   */
  static void
  handleCommand(shared_ptr<ControlCommand> command,
                const ControlCommandHandler& handler,
                const Name& prefix, const Interest& interest,
                const ndn::mgmt::ControlParameters& params,
                ndn::mgmt::CommandContinuation done);

  /**
   * @brief Generates the relative prefix for a handler by appending the verb name to the module name.
   *
   * @param verb the verb name
   * @return the generated relative prefix
   */
  PartialName
  makeRelPrefix(const std::string& verb)
  {
    return PartialName(m_module).append(verb);
  }

private:
  std::string m_module;
  Dispatcher& m_dispatcher;
  CommandAuthenticator* m_authenticator = nullptr;
};

template<typename Command>
inline void
ManagerBase::registerCommandHandler(const std::string& verb,
                                    const ControlCommandHandler& handler)
{
  auto command = make_shared<Command>();

  m_dispatcher.addControlCommand<ControlParameters>(
    makeRelPrefix(verb),
    makeAuthorization(verb),
    bind(&ManagerBase::validateParameters, std::cref(*command), _1),
    bind(&ManagerBase::handleCommand, command, handler, _1, _2, _3, _4));
}

} // namespace nfd

#endif // NFD_DAEMON_MGMT_MANAGER_BASE_HPP
