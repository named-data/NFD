/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#ifndef NFD_RIB_AUTO_PREFIX_PROPAGATOR_HPP
#define NFD_RIB_AUTO_PREFIX_PROPAGATOR_HPP

#include "rib.hpp"
#include "core/config-file.hpp"
#include "propagated-entry.hpp"

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>
#include <ndn-cxx/mgmt/nfd/command-options.hpp>
#include <ndn-cxx/util/signal.hpp>

namespace nfd {
namespace rib {

/** @brief provides automatic prefix propagation feature
 *
 * The AutoPrefixPropagator monitors changes to local RIB, and registers prefixes onto a
 * connected gateway router (HUB), so that Interests under propagated prefixes will be forwarded
 * toward the local host by the HUB.
 *
 * The route origin of propagated prefix is CLIENT, as shown on the HUB.
 *
 * Connectivity to a HUB is indicated with a special RIB entry "ndn:/localhop/nfd".
 * Currently, the AutoPrefixPropagator can process the connection to at most one HUB.
 *
 * For every RIB entry except "ndn:/localhop/nfd" and those starting with "ndn:/localhost", the
 * AutoPrefixPropagator queries the local KeyChain for signing identities that is authorized
 * to sign a prefix registration command of a prefix of the RIB prefix.
 *
 * If one or more signing identities are found, the identity that can sign a prefix registration
 * command of the shortest prefix is chosen, and the AutoPrefixPropagator will attempt to
 * propagte a prefix to the HUB using the shortest prefix. It's noteworthy that no route flags will
 * be inherited from the local registration.
 * If no signing identity is available, no prefix of the RIB entry is propagated to the HUB.
 *
 * When a RIB entry is erased, the corresponding propagated entry would be revoked,
 * unless another local RIB entry is causing the propagation of that prefix.
 *
 * After a successful propagation, the AutoPrefixPropagator will refresh the propagation
 * periodically by resending the registration command.
 *
 * In case of a failure or timeout in a registration command, the command will be retried with an
 * exponential back-off strategy.
 *
 * The AutoPrefixPropagator can be configured in NFD configuration file, at the
 * rib.auto_prefix_propagate section.
 */
class AutoPrefixPropagator : noncopyable
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

  AutoPrefixPropagator(ndn::nfd::Controller& controller,
                       ndn::KeyChain& keyChain,
                       Rib& rib);
  /**
   * @brief load the "auto_prefix_propagate" section from config file
   *
   * @param configSection the sub section in "rib" section.
   */
  void
  loadConfig(const ConfigSection& configSection);

  /**
   * @brief enable automatic prefix propagation
   */
  void
  enable();

  /**
   * @brief disable automatic prefix propagation
   */
  void
  disable();

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // helpers
  /**
   * When a local RIB prefix triggers prefix propagation, we propagate the shortest identity that
   * can sign this prefix to the HUB.
   *
   * Thus, the propagated prefix does not equal to local RIB prefix.
   * So it needs separate storage instead of storing within the RIB.
   *
   * the propagated prefix is used as the key to retrive the corresponding entry.
   */
  typedef std::unordered_map<Name, PropagatedEntry> PropagatedEntryList;
  typedef PropagatedEntryList::iterator PropagatedEntryIt;

  /**
   * @brief parameters used by the registration commands for prefix propagation
   *
   * consists of a ControlParameters and a CommandOptions, as well as a bool variable indicates
   * whether this set of parameters is valid (i.e., the signing identity exists)
   */
  struct PrefixPropagationParameters
  {
    bool isValid;
    ndn::nfd::ControlParameters parameters;
    ndn::nfd::CommandOptions options;
  };

  /**
   * @brief get the required parameters for prefix propagation.
   *
   * given a local RIB prefix @p localRibPrefix, find out, in local KeyChain, a proper identity
   * whose namespace covers the input prefix. If there is no desired identity, return a invalid
   * PrefixPropagationParameters.
   *
   * Otherwise, set the selected identity as the signing identity in CommandOptions. Meanwhile,
   * set this identity (or its prefix with the last component eliminated if it ends with "nrd")
   * as the name of ControlParameters.
   *
   * @return the PrefixPropagationParameters.
   */
  PrefixPropagationParameters
  getPrefixPropagationParameters(const Name& localRibPrefix);

  /**
   * @brief check whether current propagated prefix still works
   *
   * the current propagated prefix @p prefix will still work if and only if its corresponding
   * signing identity still exists and there is no better signing identity covering it.
   *
   * @return true if current propagated prefix still works, otherwise false
   */
  bool
  doesCurrentPropagatedPrefixWork(const Name& prefix);

  /**
   * @brief process re-propagation for the given propagated entry
   *
   * This function may be invoked in 3 cases:
   * 1. After the hub is connected to redo some propagations.
   * 2. On refresh timer to handle refresh requests.
   * 3. On retry timer to handle retry requests.
   *
   * @param entryIt the propagated entry need to do re-propagation
   * @param parameters the ControlParameters used by registration commands for propagation
   * @param options the CommandOptions used for registration commands for propagation
   * @param retryWaitTime the current wait time before retrying propagation
   */
  void
  redoPropagation(PropagatedEntryIt entryIt,
                  const ndn::nfd::ControlParameters& parameters,
                  const ndn::nfd::CommandOptions& options,
                  time::seconds retryWaitTime);

private:
  /**
   * @brief send out the registration command for propagation
   *
   * Before sending out the command, two events, for refresh and retry respectively, are
   * established (but not scheduled) and assigned to two callbacks, afterPropagateSucceed and
   * afterPropagateFail respectively.
   *
   * The retry event requires an argument to define the retry wait time, which is calculated
   * according to the back-off policy based on current retry wait time @p retryWaitTime and the
   * maxRetryWait.
   *
   * The baseRetryWait and maxRetryWait used in back-off strategy are all configured at
   * rib.auto_prefix_propagate.base_retry_wait and
   * rib.auto_prefix_propagate.max_retry_wait respectively

   * @param parameters the ControlParameters used by the registration command for propagation
   * @param options the CommandOptions used by the registration command for propagation
   * @param retryWaitTime the current wait time before retrying propagation
   */
  void
  advertise(const ndn::nfd::ControlParameters& parameters,
            const ndn::nfd::CommandOptions& options,
            time::seconds retryWaitTime);

  /**
   * @brief send out the unregistration command to revoke the corresponding propagation.
   *
   * @param parameters the ControlParameters used by the unregistration command for revocation
   * @param options the CommandOptions used by the unregistration command for revocation
   * @param retryWaitTime the current wait time before retrying propagation
   */
  void
  withdraw(const ndn::nfd::ControlParameters& parameters,
           const ndn::nfd::CommandOptions& options,
           time::seconds retryWaitTime);

  /**
   * @brief invoked when Rib::afterInsertEntry signal is emitted
   *
   * step1: if the local RIB prefix @param prefix is for local use only, return
   * step2: if the local RIB prefix @param prefix is a hub prefix, invoke afterHubConnect
   * step3: if no valid PrefixPropagationParameters can be found for the local RIB prefix
   *        @param prefix, or the propagated prefix has already been processed, return
   * step4: invoke afterRibInsert
   */
  void
  afterInsertRibEntry(const Name& prefix);

  /**
   * @brief invoked when Rib::afterEraseEntry signal is emitted
   *
   * step1: if local RIB prefix @param prefix is for local use only, return
   * step2: if local RIB prefix @param prefix is a hub prefix, invoke afterHubDisconnect
   * step3: if no valid PrefixPropagationParameters can be found for local RIB prefix
   *        @param prefix, or there are other local RIB prefixes can be covered by the propagated
   *        prefix, return
   * step4: invoke afterRibErase
   */
  void
  afterEraseRibEntry(const Name& prefix);

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // PropagatedEntry state changes
  /**
   * @brief invoked after rib insertion for non-hub prefixes
   * @pre the PropagatedEntry is in RELEASED state
   * @post the PropagatedEntry will be in NEW state
   *
   * @param parameters the ControlParameters used by registration commands for propagation
   * @param options the CommandOptions used by registration commands for propagation
   */
  void
  afterRibInsert(const ndn::nfd::ControlParameters& parameters,
                 const ndn::nfd::CommandOptions& options);

  /**
   * @brief invoked after rib deletion for non-hub prefixes
   * @pre the PropagatedEntry is not in RELEASED state
   * @post the PropagatedEntry will be in RELEASED state
   *
   * @param parameters the ControlParameters used by registration commands for propagation
   * @param options the CommandOptions used by registration commands for propagation
   */
  void
  afterRibErase(const ndn::nfd::ControlParameters& parameters,
                const ndn::nfd::CommandOptions& options);

  /**
   * @brief invoked after the hub is connected
   * @pre the PropagatedEntry is in NEW state or RELEASED state
   * @post the PropagatedEntry will be in PROPAGATING state or keep in RELEASED state
   *
   * call redoPropagation for each propagated entry.
   */
  void
  afterHubConnect();

  /**
   * @brief invoked after the hub is disconnected
   * @pre the PropagatedEntry is not in NEW state
   * @post the PropagatedEntry will be in NEW state or keep in RELEASED state
   *
   * for each propagated entry, switch its state to NEW and cancel its event.
   */
  void
  afterHubDisconnect();

  /**
   * @brief invoked after propagation succeeds
   * @pre the PropagatedEntry is in PROPAGATING state or RELEASED state
   * @post the PropagatedEntry will be in PROPAGATED state or keep in RELEASED state
   *
   * If the PropagatedEntry does not exist (in RELEASED state), an unregistration command is
   * sent immediately for revocation.
   *
   * If the PropagatedEntry is in PROPAGATING state, switch it to PROPAGATED, and schedule a
   * refresh timer to redo propagation after a duration, which is configured at
   * rib.auto_prefix_propagate.refresh_interval.
   *
   * Otherwise, make a copy of the ControlParameters @p parameters, unset its Cost field, and then
   * invoke withdraw with this new ControlParameters.
   *
   * @param parameters the ControlParameters used by the registration command for propagation
   * @param options the CommandOptions used by the registration command for propagation
   * @param refreshEvent the event of refreshing propagation
   */
  void
  afterPropagateSucceed(const ndn::nfd::ControlParameters& parameters,
                        const ndn::nfd::CommandOptions& options,
                        const scheduler::EventCallback& refreshEvent);

  /**
   * @brief invoked after propagation fails.
   * @pre the PropagatedEntry is in PROPAGATING state or RELEASED state
   * @post the PropagatedEntry will be in PROPAGATE_FAIL state or keep in RELEASED state
   *
   * If the PropagatedEntry still exists, schedule a retry timer to redo propagation
   * after a duration defined by current retry time @p retryWaitTime
   *
   * @param response ControlResponse from remote NFD-RIB
   * @param parameters the ControlParameters used by the registration command for propagation
   * @param options the CommandOptions used by registration command for propagation
   * @param retryWaitTime the current wait time before retrying propagation
   * @param retryEvent the event of retrying propagation
   */
  void
  afterPropagateFail(const ndn::nfd::ControlResponse& response,
                     const ndn::nfd::ControlParameters& parameters,
                     const ndn::nfd::CommandOptions& options,
                     time::seconds retryWaitTime,
                     const scheduler::EventCallback& retryEvent);

  /**
   * @brief invoked after revocation succeeds
   * @pre the PropagatedEntry is not in NEW state
   * @post the PropagatedEntry will be in PROPAGATING state, or keep in PROPAGATE_FAIL state,
   * or keep in RELEASED state
   *
   * If the PropagatedEntry still exists and is not in PROPAGATE_FAIL state, switch it to
   * PROPAGATING. Then make a copy of the ControlParameters @p parameters, reset its Cost, and
   * invoke advertise with this new ControlParameters.
   *
   * @param parameters the ControlParameters used by the unregistration command for revocation
   * @param options the CommandOptions used by the unregistration command for revocation
   * @param retryWaitTime the current wait time before retrying propagation
   */
  void
  afterRevokeSucceed(const ndn::nfd::ControlParameters& parameters,
                     const ndn::nfd::CommandOptions& options,
                     time::seconds retryWaitTime);

  /**
   * @brief invoked after revocation fails.
   *
   * @param response ControlResponse from remote NFD-RIB
   * @param parameters the ControlParameters used by the unregistration command for revocation
   * @param options the CommandOptions used by the unregistration command for revocation
   */
  void
  afterRevokeFail(const ndn::nfd::ControlResponse& response,
                  const ndn::nfd::ControlParameters& parameters,
                  const ndn::nfd::CommandOptions& options);

  /**
   * @brief invoked when the refresh timer is triggered.
   * @pre the PropagatedEntry is in PROPAGATED state
   * @post the PropagatedEntry will be in PROPAGATING state
   *
   * call redoPropagation to handle this refresh request
   *
   * @param parameters the ControlParameters used by registration commands for propagation
   * @param options the CommandOptions used by registration commands for propagation
   */
  void
  onRefreshTimer(const ndn::nfd::ControlParameters& parameters,
                 const ndn::nfd::CommandOptions& options);

  /**
   * @brief invoked when the retry timer is triggered.
   * @pre the PropagatedEntry is in PROPAGATE_FAIL state
   * @post the PropagatedEntry will be in PROPAGATING state
   *
   * call redoPropagation to handle this retry request
   *
   * @param parameters the ControlParameters used by registration commands for propagation
   * @param options the CommandOptions used by registration commands for propagation
   * @param retryWaitTime the current wait time before retrying propagation
   */
  void
  onRetryTimer(const ndn::nfd::ControlParameters& parameters,
               const ndn::nfd::CommandOptions& options,
               time::seconds retryWaitTime);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  ndn::nfd::Controller& m_nfdController;
  ndn::KeyChain& m_keyChain;
  Rib& m_rib;
  ndn::util::signal::ScopedConnection m_afterInsertConnection;
  ndn::util::signal::ScopedConnection m_afterEraseConnection;
  ndn::nfd::ControlParameters m_controlParameters;
  ndn::nfd::CommandOptions m_commandOptions;
  time::seconds m_refreshInterval;
  time::seconds m_baseRetryWait;
  time::seconds m_maxRetryWait;
  bool m_hasConnectedHub;
  PropagatedEntryList m_propagatedEntries;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_AUTO_PREFIX_PROPAGATOR_HPP
