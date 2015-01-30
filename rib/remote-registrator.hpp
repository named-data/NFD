/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#ifndef NFD_RIB_REMOTE_REGISTRATOR_HPP
#define NFD_RIB_REMOTE_REGISTRATOR_HPP

#include "rib.hpp"
#include "core/config-file.hpp"
#include "rib-status-publisher.hpp"

#include <unordered_map>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/management/nfd-controller.hpp>
#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-control-parameters.hpp>
#include <ndn-cxx/management/nfd-command-options.hpp>
#include <ndn-cxx/util/signal.hpp>

namespace nfd {
namespace rib {

/**
 * @brief define the RemoteRegistrator class, which handles
 *        the registration/unregistration to remote hub(s).
 */
class RemoteRegistrator : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  RemoteRegistrator(ndn::nfd::Controller& controller,
                    ndn::KeyChain& keyChain,
                    Rib& rib);

  ~RemoteRegistrator();

  /**
   * @brief load the "remote_register" section from config file
   *
   * @param configSection the sub section in "rib" section.
   */
  void
  loadConfig(const ConfigSection& configSection);

  /**
   * @brief enable remote registration/unregistration.
   *
   */
  void
  enable();

  /**
   * @brief disable remote registration/unregistration.
   *
   */
  void
  disable();

  /**
   * @brief register a prefix to remote hub(s).
   *
   * For the input prefix, we find the longest identity
   * in the key-chain that can sign it, and then
   * register this identity to remote hub(s).
   *
   * @param prefix the prefix being registered in local RIB.
   */
  void
  registerPrefix(const Name& prefix);

  /**
   * @brief unregister a prefix from remote hub(s).
   *
   * For the input prefix, if the longest identity can sign it
   * is already registered remotely, that identity should be
   * unregistered from remote hub(s).
   *
   * @param prefix the prefix being unregistered in local RIB.
   */
  void
  unregisterPrefix(const Name& prefix);

private:
  /**
   * @brief find the most proper identity that can sign the
   *        registration/unregistration command for the input prefix.
   *
   * @return the identity and the length of the longest match to the
   *         input prefix.
   *
   * @retval { ignored, 0 } no matching identity
   */
  std::pair<Name, size_t>
  findIdentityForRegistration(const Name& prefix);

  /**
   * @brief make and send the remote registration command.
   *
   * @param nRetries remaining number of retries.
   */
  void
  startRegistration(const ndn::nfd::ControlParameters& parameters,
                    const ndn::nfd::CommandOptions& options,
                    int nRetries);

  /**
   * @brief make and send the remote unregistration command.
   *
   * @param nRetries remaining number of retries.
   */
  void
  startUnregistration(const ndn::nfd::ControlParameters& parameters,
                      const ndn::nfd::CommandOptions& options,
                      int nRetries);
  /**
   * @brief refresh the remotely registered entry if registration
   *        successes by re-sending the registration command.
   *
   * The interval of sending refresh command is defined in the
   * "remote_register" section of the config file.
   *
   * @param parameters the same paremeters from startRegistration.
   * @param options the same options as in startRegistration.
   */
  void
  onRegSuccess(const ndn::nfd::ControlParameters& parameters,
               const ndn::nfd::CommandOptions& options);

  /**
   * @brief retry to send registration command if registration fails.
   *
   * the number of retries is defined in the "remote_register"
   * section of the config file.
   *
   * @param code error code.
   * @param reason error reason in string.
   * @param parameters the same paremeters from startRegistration.
   * @param options the same options from startRegistration.
   * @param nRetries remaining number of retries.
   */
  void
  onRegFailure(uint32_t code, const std::string& reason,
               const ndn::nfd::ControlParameters& parameters,
               const ndn::nfd::CommandOptions& options,
               int nRetries);

  void
  onUnregSuccess(const ndn::nfd::ControlParameters& parameters,
                 const ndn::nfd::CommandOptions& options);

  /**
   * @brief retry to send unregistration command if registration fails.
   *
   * the number of retries is defined in the "remote_register"
   * section of the config file.
   *
   * @param code error code.
   * @param reason error reason in string.
   * @param parameters the same paremeters as in startRegistration.
   * @param options the same options as in startRegistration.
   * @param nRetries remaining number of retries.
   */
  void
  onUnregFailure(uint32_t code, const std::string& reason,
                 const ndn::nfd::ControlParameters& parameters,
                 const ndn::nfd::CommandOptions& options,
                 int nRetries);

  /**
   * @brief re-register all prefixes
   *
   * This is called when a HUB connection is established.
   */
  void
  redoRegistration();

  /**
   * @brief clear all refresh events
   *
   * This is called when all HUB connections are lost.
   */
  void
  clearRefreshEvents();

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /**
   * When a locally registered prefix triggles remote
   * registration, we actually register the longest
   * identity that can sign this prefix to remote hub(s).
   *
   * Thus, the remotely reigstered prefix does not equal
   * to Route Name. So it needs seperate sotrage instead
   * of storing within the RIB.
   */
  typedef std::unordered_map<Name, scheduler::EventId> RegisteredList;
  typedef RegisteredList::iterator RegisteredEntryIt;
  typedef RegisteredList::value_type RegisteredEntry;
  RegisteredList m_regEntries;

private:
  ndn::nfd::Controller& m_nfdController;
  ndn::KeyChain& m_keyChain;
  Rib& m_rib;
  ndn::util::signal::ScopedConnection m_afterInsertConnection;
  ndn::util::signal::ScopedConnection m_afterEraseConnection;
  ndn::nfd::ControlParameters m_controlParameters;
  ndn::nfd::CommandOptions m_commandOptions;
  time::seconds m_refreshInterval;
  bool m_hasConnectedHub;
  int m_nRetries;

  static const Name LOCAL_REGISTRATION_PREFIX; // /localhost
  static const Name REMOTE_HUB_PREFIX; // /localhop/nfd
  static const name::Component IGNORE_COMMPONENT; // rib
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_REMOTE_REGISTRATOR_HPP
