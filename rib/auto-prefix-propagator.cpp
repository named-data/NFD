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

#include "auto-prefix-propagator.hpp"
#include "core/logger.hpp"
#include "core/scheduler.hpp"
#include <ndn-cxx/security/pib/identity.hpp>
#include <ndn-cxx/security/pib/identity-container.hpp>
#include <ndn-cxx/security/pib/pib.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <vector>

namespace nfd {
namespace rib {

NFD_LOG_INIT("AutoPrefixPropagator");

using ndn::nfd::ControlParameters;
using ndn::nfd::CommandOptions;

const Name LOCAL_REGISTRATION_PREFIX("/localhost");
const Name LINK_LOCAL_NFD_PREFIX("/localhop/nfd");
const name::Component IGNORE_COMMPONENT("nrd");
const time::seconds PREFIX_PROPAGATION_DEFAULT_REFRESH_INTERVAL = time::seconds(25);
const time::seconds PREFIX_PROPAGATION_MAX_REFRESH_INTERVAL = time::seconds(600);
const time::seconds PREFIX_PROPAGATION_DEFAULT_BASE_RETRY_WAIT = time::seconds(50);
const time::seconds PREFIX_PROPAGATION_DEFAULT_MAX_RETRY_WAIT = time::seconds(3600);
const uint64_t PREFIX_PROPAGATION_DEFAULT_COST = 15;
const time::milliseconds PREFIX_PROPAGATION_DEFAULT_TIMEOUT = time::milliseconds(10000);

AutoPrefixPropagator::AutoPrefixPropagator(ndn::nfd::Controller& controller,
                                           ndn::KeyChain& keyChain,
                                           Rib& rib)
  : m_nfdController(controller)
  , m_keyChain(keyChain)
  , m_rib(rib)
  , m_refreshInterval(PREFIX_PROPAGATION_DEFAULT_REFRESH_INTERVAL)
  , m_baseRetryWait(PREFIX_PROPAGATION_DEFAULT_BASE_RETRY_WAIT)
  , m_maxRetryWait(PREFIX_PROPAGATION_DEFAULT_MAX_RETRY_WAIT)
  , m_hasConnectedHub(false)
{
}

void
AutoPrefixPropagator::loadConfig(const ConfigSection& configSection)
{
  m_refreshInterval = PREFIX_PROPAGATION_DEFAULT_REFRESH_INTERVAL;
  m_baseRetryWait = PREFIX_PROPAGATION_DEFAULT_BASE_RETRY_WAIT;
  m_maxRetryWait = PREFIX_PROPAGATION_DEFAULT_MAX_RETRY_WAIT;

  m_controlParameters
     .setCost(PREFIX_PROPAGATION_DEFAULT_COST)
     .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT)// set origin to client.
     .setFaceId(0);// the remote hub will take the input face as the faceId.

   m_commandOptions
     .setPrefix(LINK_LOCAL_NFD_PREFIX)
     .setTimeout(PREFIX_PROPAGATION_DEFAULT_TIMEOUT);

  NFD_LOG_INFO("Load auto_prefix_propagate section in rib section");

  for (auto&& i : configSection) {
    if (i.first == "cost") {
      m_controlParameters.setCost(i.second.get_value<uint64_t>());
    }
    else if (i.first == "timeout") {
      m_commandOptions.setTimeout(time::milliseconds(i.second.get_value<size_t>()));
    }
    else if (i.first == "refresh_interval") {
      m_refreshInterval = std::min(PREFIX_PROPAGATION_MAX_REFRESH_INTERVAL,
                                   time::seconds(i.second.get_value<size_t>()));
    }
    else if (i.first == "base_retry_wait") {
      m_baseRetryWait = time::seconds(i.second.get_value<size_t>());
    }
    else if (i.first == "max_retry_wait") {
      m_maxRetryWait = time::seconds(i.second.get_value<size_t>());
    }
    else {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option \"" + i.first +
                                              "\" in \"auto_prefix_propagate\" section"));
    }
  }
}

void
AutoPrefixPropagator::enable()
{
  m_afterInsertConnection =
    m_rib.afterInsertEntry.connect(bind(&AutoPrefixPropagator::afterInsertRibEntry, this, _1));
  m_afterEraseConnection =
    m_rib.afterEraseEntry.connect(bind(&AutoPrefixPropagator::afterEraseRibEntry, this, _1));
}

void
AutoPrefixPropagator::disable()
{
  m_afterInsertConnection.disconnect();
  m_afterEraseConnection.disconnect();
}

AutoPrefixPropagator::PrefixPropagationParameters
AutoPrefixPropagator::getPrefixPropagationParameters(const Name& localRibPrefix)
{
  // shortest prefix matching to all identies.
  Name propagatedPrefix;
  ndn::security::pib::Identity signingIdentity;
  bool isFound = false;
  for (auto&& identity : m_keyChain.getPib().getIdentities()) {
    Name idName = identity.getName();
    Name prefix = !idName.empty() && IGNORE_COMMPONENT == idName.at(-1) ?
                  idName.getPrefix(-1) : idName;
    if (prefix.isPrefixOf(localRibPrefix) && (!isFound || prefix.size() < propagatedPrefix.size())) {
      isFound = true;
      propagatedPrefix = prefix;
      signingIdentity = identity;
    }
  }

  PrefixPropagationParameters propagateParameters;
  if (!isFound) {
    propagateParameters.isValid = false;
  }
  else {
    propagateParameters.isValid = true;
    propagateParameters.parameters = m_controlParameters;
    propagateParameters.options = m_commandOptions;
    propagateParameters.parameters.setName(propagatedPrefix);
    propagateParameters.options.setSigningInfo(ndn::security::signingByIdentity(signingIdentity));
  }

  return propagateParameters;
}

void
AutoPrefixPropagator::afterInsertRibEntry(const Name& prefix)
{
  if (LOCAL_REGISTRATION_PREFIX.isPrefixOf(prefix)) {
    NFD_LOG_INFO("local registration only for " << prefix);
    return;
  }

  if (prefix == LINK_LOCAL_NFD_PREFIX) {
    NFD_LOG_INFO("this is a prefix registered by some hub: " << prefix);

    m_hasConnectedHub = true;
    return afterHubConnect();
  }

  auto propagateParameters = getPrefixPropagationParameters(prefix);
  if (!propagateParameters.isValid) {
    NFD_LOG_INFO("no signing identity available for: " << prefix);
    return;
  }

  auto entryIt = m_propagatedEntries.find(propagateParameters.parameters.getName());
  if (entryIt != m_propagatedEntries.end()) {
    // in addition to PROPAGATED and PROPAGATE_FAIL, the state may also be NEW,
    // if there is no hub connected to propagate this prefix.
    if (entryIt->second.isNew()) {
      NFD_LOG_INFO("no hub connected to propagate "
                   << propagateParameters.parameters.getName());
    }
    else {
      NFD_LOG_INFO("prefix has already been propagated: "
                   << propagateParameters.parameters.getName());
    }
    return;
  }

  afterRibInsert(propagateParameters.parameters, propagateParameters.options);
}

void
AutoPrefixPropagator::afterEraseRibEntry(const Name& prefix)
{
  if (LOCAL_REGISTRATION_PREFIX.isPrefixOf(prefix)) {
    NFD_LOG_INFO("local unregistration only for " << prefix);
    return;
  }

  if (prefix == LINK_LOCAL_NFD_PREFIX) {
    NFD_LOG_INFO("disconnected to hub with prefix: " << prefix);

    m_hasConnectedHub = false;
    return afterHubDisconnect();
  }

  auto propagateParameters = getPrefixPropagationParameters(prefix);
  if (!propagateParameters.isValid) {
    NFD_LOG_INFO("no signing identity available for: " << prefix);
    return;
  }

  auto entryIt = m_propagatedEntries.find(propagateParameters.parameters.getName());
  if (entryIt == m_propagatedEntries.end()) {
    NFD_LOG_INFO("prefix has not been propagated yet: "
                 << propagateParameters.parameters.getName());
    return;
  }

  for (auto&& ribTableEntry : m_rib) {
    if (propagateParameters.parameters.getName().isPrefixOf(ribTableEntry.first) &&
        propagateParameters.options.getSigningInfo().getSignerName() ==
        getPrefixPropagationParameters(ribTableEntry.first)
          .options.getSigningInfo().getSignerName()) {
      NFD_LOG_INFO("should be kept for another RIB entry: " << ribTableEntry.first);
      return;
    }
  }

  afterRibErase(propagateParameters.parameters.unsetCost(), propagateParameters.options);
}

bool
AutoPrefixPropagator::doesCurrentPropagatedPrefixWork(const Name& prefix)
{
  auto propagateParameters = getPrefixPropagationParameters(prefix);
  if (!propagateParameters.isValid) {
    // no identity can sign the input prefix
    return false;
  }

  // there is at least one identity can sign the input prefix, so the prefix selected for
  // propagation (i.e., propagateParameters.parameters.getName()) must be a prefix of the input
  // prefix. Namely it's either equal to the input prefix or a better choice.
  return propagateParameters.parameters.getName().size() == prefix.size();
}

void
AutoPrefixPropagator::redoPropagation(PropagatedEntryIt entryIt,
                                      const ControlParameters& parameters,
                                      const CommandOptions& options,
                                      time::seconds retryWaitTime)
{
  if (doesCurrentPropagatedPrefixWork(parameters.getName())) {
    // PROPAGATED / PROPAGATE_FAIL --> PROPAGATING
    entryIt->second.startPropagation();
    return advertise(parameters, options, retryWaitTime);
  }

  NFD_LOG_INFO("current propagated prefix does not work any more");
  m_propagatedEntries.erase(entryIt);

  // re-handle all locally RIB entries that can be covered by this propagated prefix
  for (auto&& ribTableEntry : m_rib) {
    if (parameters.getName().isPrefixOf(ribTableEntry.first)) {
      afterInsertRibEntry(ribTableEntry.first);
    }
  }
}

void
AutoPrefixPropagator::advertise(const ControlParameters& parameters,
                                const CommandOptions& options,
                                time::seconds retryWaitTime)
{
  NFD_LOG_INFO("advertise " << parameters.getName());

  scheduler::EventCallback refreshEvent =
    bind(&AutoPrefixPropagator::onRefreshTimer, this, parameters, options);
  scheduler::EventCallback retryEvent =
    bind(&AutoPrefixPropagator::onRetryTimer, this, parameters, options,
         std::min(m_maxRetryWait, retryWaitTime * 2));

  m_nfdController.start<ndn::nfd::RibRegisterCommand>(
     parameters,
     bind(&AutoPrefixPropagator::afterPropagateSucceed, this, parameters, options, refreshEvent),
     bind(&AutoPrefixPropagator::afterPropagateFail,
          this, _1, parameters, options, retryWaitTime, retryEvent),
     options);
}

void
AutoPrefixPropagator::withdraw(const ControlParameters& parameters,
                               const CommandOptions& options,
                               time::seconds retryWaitTime)
{
  NFD_LOG_INFO("withdraw " << parameters.getName());

  m_nfdController.start<ndn::nfd::RibUnregisterCommand>(
     parameters,
     bind(&AutoPrefixPropagator::afterRevokeSucceed, this, parameters, options, retryWaitTime),
     bind(&AutoPrefixPropagator::afterRevokeFail, this, _1, parameters, options),
     options);
}

void
AutoPrefixPropagator::afterRibInsert(const ControlParameters& parameters,
                                     const CommandOptions& options)
{
  BOOST_ASSERT(m_propagatedEntries.find(parameters.getName()) == m_propagatedEntries.end());

  // keep valid entries although there is no connectivity to hub
  auto& entry = m_propagatedEntries[parameters.getName()]
    .setSigningIdentity(options.getSigningInfo().getSignerName());

  if (!m_hasConnectedHub) {
    NFD_LOG_INFO("no hub connected to propagate " << parameters.getName());
    return;
  }

  // NEW --> PROPAGATING
  entry.startPropagation();
  advertise(parameters, options, m_baseRetryWait);
}

void
AutoPrefixPropagator::afterRibErase(const ControlParameters& parameters,
                                    const CommandOptions& options)
{
  auto entryIt = m_propagatedEntries.find(parameters.getName());
  BOOST_ASSERT(entryIt != m_propagatedEntries.end());

  bool hasPropagationSucceeded = entryIt->second.isPropagated();

  // --> "RELEASED"
  m_propagatedEntries.erase(entryIt);

  if (!m_hasConnectedHub) {
    NFD_LOG_INFO("no hub connected to revoke propagation of " << parameters.getName());
    return;
  }

  if (!hasPropagationSucceeded) {
    NFD_LOG_INFO("propagation has not succeeded: " << parameters.getName());
    return;
  }

  withdraw(parameters, options, m_baseRetryWait);
}

void
AutoPrefixPropagator::afterHubConnect()
{
  NFD_LOG_INFO("redo " << m_propagatedEntries.size()
                       << " propagations when new Hub connectivity is built.");

  std::vector<PropagatedEntryIt> regEntryIterators;
  for (auto it = m_propagatedEntries.begin() ; it != m_propagatedEntries.end() ; it ++) {
    BOOST_ASSERT(it->second.isNew());
    regEntryIterators.push_back(it);
  }

  for (auto&& it : regEntryIterators) {
    auto parameters = m_controlParameters;
    auto options = m_commandOptions;

    redoPropagation(it,
                     parameters.setName(it->first),
                     options.setSigningInfo(signingByIdentity(it->second.getSigningIdentity())),
                     m_baseRetryWait);
  }
}

void
AutoPrefixPropagator::afterHubDisconnect()
{
  for (auto&& entry : m_propagatedEntries) {
    // --> NEW
    BOOST_ASSERT(!entry.second.isNew());
    entry.second.initialize();
  }
}

void
AutoPrefixPropagator::afterPropagateSucceed(const ControlParameters& parameters,
                                            const CommandOptions& options,
                                            const scheduler::EventCallback& refreshEvent)
{
  NFD_LOG_TRACE("success to propagate " << parameters.getName());

  auto entryIt = m_propagatedEntries.find(parameters.getName());
  if (entryIt == m_propagatedEntries.end()) {
    // propagation should be revoked if this entry has been erased (i.e., be in RELEASED state)
    NFD_LOG_DEBUG("Already erased!");
    ControlParameters newParameters = parameters;
    return withdraw(newParameters.unsetCost(), options, m_baseRetryWait);
  }

  // PROPAGATING --> PROPAGATED
  BOOST_ASSERT(entryIt->second.isPropagating());
  entryIt->second.succeed(scheduler::schedule(m_refreshInterval, refreshEvent));
}

void
AutoPrefixPropagator::afterPropagateFail(const ndn::nfd::ControlResponse& response,
                                         const ControlParameters& parameters,
                                         const CommandOptions& options,
                                         time::seconds retryWaitTime,
                                         const scheduler::EventCallback& retryEvent)
{
  NFD_LOG_TRACE("fail to propagate " << parameters.getName()
                                     << "\n\t reason:" << response.getText()
                                     << "\n\t retry wait time: " << retryWaitTime);

  auto entryIt = m_propagatedEntries.find(parameters.getName());
  if (entryIt == m_propagatedEntries.end()) {
    // current state is RELEASED
    return;
  }

  // PROPAGATING --> PROPAGATE_FAIL
  BOOST_ASSERT(entryIt->second.isPropagating());
  entryIt->second.fail(scheduler::schedule(retryWaitTime, retryEvent));
}

void
AutoPrefixPropagator::afterRevokeSucceed(const ControlParameters& parameters,
                                         const CommandOptions& options,
                                         time::seconds retryWaitTime)
{
  NFD_LOG_TRACE("success to revoke propagation of " << parameters.getName());

  auto entryIt = m_propagatedEntries.find(parameters.getName());
  if (m_propagatedEntries.end() != entryIt && !entryIt->second.isPropagateFail()) {
    // if is not RELEASED or PROPAGATE_FAIL
    NFD_LOG_DEBUG("propagated entry still exists");

    // PROPAGATING / PROPAGATED --> PROPAGATING
    BOOST_ASSERT(!entryIt->second.isNew());
    entryIt->second.startPropagation();

    ControlParameters newParameters = parameters;
    advertise(newParameters.setCost(m_controlParameters.getCost()), options, retryWaitTime);
  }
}

void
AutoPrefixPropagator::afterRevokeFail(const ndn::nfd::ControlResponse& response,
                                      const ControlParameters& parameters,
                                      const CommandOptions& options)
{
  NFD_LOG_INFO("fail to revoke the propagation of  " << parameters.getName()
                                                     << "\n\t reason:" << response.getText());
}

void
AutoPrefixPropagator::onRefreshTimer(const ControlParameters& parameters,
                                     const CommandOptions& options)
{
  auto entryIt = m_propagatedEntries.find(parameters.getName());
  BOOST_ASSERT(entryIt != m_propagatedEntries.end() && entryIt->second.isPropagated());
  redoPropagation(entryIt, parameters, options, m_baseRetryWait);
}

void
AutoPrefixPropagator::onRetryTimer(const ControlParameters& parameters,
                                   const CommandOptions& options,
                                   time::seconds retryWaitTime)
{
  auto entryIt = m_propagatedEntries.find(parameters.getName());
  BOOST_ASSERT(entryIt != m_propagatedEntries.end() && entryIt->second.isPropagateFail());
  redoPropagation(entryIt, parameters, options, retryWaitTime);
}

} // namespace rib
} // namespace nfd
