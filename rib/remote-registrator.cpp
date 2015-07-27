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

#include "remote-registrator.hpp"
#include "core/logger.hpp"
#include "core/scheduler.hpp"
#include <ndn-cxx/security/signing-helpers.hpp>

namespace nfd {
namespace rib {

NFD_LOG_INIT("RemoteRegistrator");

using ndn::nfd::ControlParameters;
using ndn::nfd::CommandOptions;

const Name RemoteRegistrator::LOCAL_REGISTRATION_PREFIX = "/localhost";
const Name RemoteRegistrator::REMOTE_HUB_PREFIX = "/localhop/nfd";
const name::Component RemoteRegistrator::IGNORE_COMMPONENT("rib");

RemoteRegistrator::RemoteRegistrator(ndn::nfd::Controller& controller,
                                     ndn::KeyChain& keyChain,
                                     Rib& rib)
  : m_nfdController(controller)
  , m_keyChain(keyChain)
  , m_rib(rib)
  , m_refreshInterval(time::seconds(25))
  , m_hasConnectedHub(false)
  , m_nRetries(0)
{
}

RemoteRegistrator::~RemoteRegistrator()
{
  // cancel all periodically refresh events.
  for (auto&& entry : m_regEntries)
    {
      scheduler::cancel(entry.second);
    }
}

void
RemoteRegistrator::loadConfig(const ConfigSection& configSection)
{
  size_t cost = 15, timeout = 10000;
  size_t retry = 0;
  size_t interval = 0;
  const size_t intervalDef = 25, intervalMax = 600;

  NFD_LOG_INFO("Load remote_register section in rib section");
  for (auto&& i : configSection)
    {
      if (i.first == "cost")
        {
          cost = i.second.get_value<size_t>();
        }
      else if (i.first == "timeout")
        {
          timeout = i.second.get_value<size_t>();
        }
      else if (i.first == "retry")
        {
          retry = i.second.get_value<size_t>();
        }
      else if (i.first == "refresh_interval")
        {
          interval = i.second.get_value<size_t>();
        }
      else
        {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option \"" + i.first +
                                                  "\" in \"remote-registrator\" section"));
        }
    }

   m_controlParameters
     .setCost(cost)
     .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT)// set origin to client.
     .setFaceId(0);// the remote hub will take the input face as the faceId.

   m_commandOptions
     .setPrefix(REMOTE_HUB_PREFIX)
     .setTimeout(time::milliseconds(timeout));

   m_nRetries = retry;

   if (interval == 0)
     {
       interval = intervalDef;
     }

   interval = std::min(interval, intervalMax);

   m_refreshInterval = time::seconds(interval);
}

void
RemoteRegistrator::enable()
{
  // do remote registration after an entry is inserted into the RIB.
  m_afterInsertConnection =
    m_rib.afterInsertEntry.connect([this] (const Name& prefix) {
        registerPrefix(prefix);
      });

  // do remote unregistration after an entry is erased from the RIB.
  m_afterEraseConnection =
    m_rib.afterEraseEntry.connect([this] (const Name& prefix) {
        unregisterPrefix(prefix);
      });
}

void
RemoteRegistrator::disable()
{
  m_afterInsertConnection.disconnect();
  m_afterEraseConnection.disconnect();
}

void
RemoteRegistrator::registerPrefix(const Name& prefix)
{
  if (LOCAL_REGISTRATION_PREFIX.isPrefixOf(prefix))
    {
      NFD_LOG_INFO("local registration only for " << prefix);
      return;
    }

  bool isHubPrefix = prefix == REMOTE_HUB_PREFIX;

  if (isHubPrefix)
    {
      NFD_LOG_INFO("this is a prefix registered by some hub: " << prefix);

      m_hasConnectedHub = true;

      redoRegistration();
      return;
    }

  if (!m_hasConnectedHub)
    {
      NFD_LOG_INFO("no hub connected when registering " << prefix);
      return;
    }

  std::pair<Name, size_t> identity = findIdentityForRegistration(prefix);

  if (0 == identity.second)
    {
      NFD_LOG_INFO("no proper identity found for registering " << prefix);
      return;
    }

  Name prefixForRegistration;
  if (identity.first.size() == identity.second)
    {
      prefixForRegistration = identity.first;
    }
  else
    {
      prefixForRegistration = identity.first.getPrefix(-1);
    }

  if (m_regEntries.find(prefixForRegistration) != m_regEntries.end())
    {
      NFD_LOG_INFO("registration already in process for " << prefix);
      return;
    }

  // make copies of m_controlParameters and m_commandOptions to
  // avoid unreasonable overwriting during concurrent registration
  // and unregistration.
  ControlParameters parameters = m_controlParameters;
  CommandOptions    options    = m_commandOptions;

  startRegistration(parameters.setName(prefixForRegistration),
                    options.setSigningInfo(signingByIdentity(identity.first)),
                    m_nRetries);
}

void
RemoteRegistrator::unregisterPrefix(const Name& prefix)
{
  if (prefix == REMOTE_HUB_PREFIX)
    {
      NFD_LOG_INFO("disconnected to hub with prefix: " << prefix);

      // for phase 1: suppose there is at most one hub connected.
      // if the hub prefix has been unregistered locally, there may
      // be no connected hub.
      m_hasConnectedHub = false;

      clearRefreshEvents();
      return;
    }

  if (!m_hasConnectedHub)
    {
      NFD_LOG_INFO("no hub connected when unregistering " << prefix);
      return;
    }

  std::pair<Name, size_t> identity = findIdentityForRegistration(prefix);

  if (0 == identity.second)
    {
      NFD_LOG_INFO("no proper identity found for unregistering " << prefix);
      return;
    }

  Name prefixForRegistration;
  if (identity.first.size() == identity.second)
    {
      prefixForRegistration = identity.first;
    }
  else
    {
      prefixForRegistration = identity.first.getPrefix(-1);
    }

  RegisteredEntryIt iRegEntry = m_regEntries.find(prefixForRegistration);
  if (m_regEntries.end() == iRegEntry)
    {
      NFD_LOG_INFO("no existing entry found when unregistering " << prefix);
      return;
    }

  for (auto&& entry : m_rib)
    {
      if (prefixForRegistration.isPrefixOf(entry.first) &&
          findIdentityForRegistration(entry.first) == identity)
        {
          NFD_LOG_INFO("this identity should be kept for other rib entry: "
                       << entry.first);
          return;
        }
    }

  scheduler::cancel(iRegEntry->second);
  m_regEntries.erase(iRegEntry);

  // make copies of m_controlParameters and m_commandOptions to
  // avoid unreasonable overwriting during concurrent registration
  // and unregistration.
  ControlParameters parameters = m_controlParameters;
  CommandOptions    options    = m_commandOptions;

  startUnregistration(parameters.setName(prefixForRegistration).unsetCost(),
                      options.setSigningInfo(signingByIdentity(identity.first)),
                      m_nRetries);
}

std::pair<Name, size_t>
RemoteRegistrator::findIdentityForRegistration(const Name& prefix)
{
  std::pair<Name, size_t> candidateIdentity;
  std::vector<Name> identities;
  bool isPrefix = false;
  size_t maxLength = 0, curLength = 0;

  // get all identies from the key-cahin except the default one.
  m_keyChain.getAllIdentities(identities, false);

  // get the default identity.
  identities.push_back(m_keyChain.getDefaultIdentity());

  // longest prefix matching to all indenties.
  for (auto&& i : identities)
    {
      if (!i.empty() && IGNORE_COMMPONENT == i.at(-1))
        {
          isPrefix = i.getPrefix(-1).isPrefixOf(prefix);
          curLength = i.size() - 1;
        }
      else
        {
          isPrefix = i.isPrefixOf(prefix);
          curLength = i.size();
        }

      if (isPrefix && curLength > maxLength)
        {
          candidateIdentity.first = i;
          maxLength = curLength;
        }
    }

  candidateIdentity.second = maxLength;

  return candidateIdentity;
}

void
RemoteRegistrator::startRegistration(const ControlParameters& parameters,
                                     const CommandOptions& options,
                                     int nRetries)
{
  NFD_LOG_INFO("start register " << parameters.getName());

  m_nfdController.start<ndn::nfd::RibRegisterCommand>(
     parameters,
     bind(&RemoteRegistrator::onRegSuccess,
          this, parameters, options),
     bind(&RemoteRegistrator::onRegFailure,
          this, _1, _2, parameters, options, nRetries),
     options);
}

void
RemoteRegistrator::startUnregistration(const ControlParameters& parameters,
                                       const CommandOptions& options,
                                       int nRetries)
{
  NFD_LOG_INFO("start unregister " << parameters.getName());

  m_nfdController.start<ndn::nfd::RibUnregisterCommand>(
     parameters,
     bind(&RemoteRegistrator::onUnregSuccess,
          this, parameters, options),
     bind(&RemoteRegistrator::onUnregFailure,
          this, _1, _2, parameters, options, nRetries),
     options);
}

void
RemoteRegistrator::onRegSuccess(const ControlParameters& parameters,
                                const CommandOptions& options)
{
  NFD_LOG_INFO("success to register " << parameters.getName());

  RegisteredEntryIt iRegEntry = m_regEntries.find(parameters.getName());

  if (m_regEntries.end() != iRegEntry)
    {
      NFD_LOG_DEBUG("Existing Entry: (" << iRegEntry->first
                                        << ", " << iRegEntry->second
                                        << ")");

      scheduler::cancel(iRegEntry->second);
      iRegEntry->second = scheduler::schedule(
                            m_refreshInterval,
                            bind(&RemoteRegistrator::startRegistration,
                                 this, parameters, options, m_nRetries));
    }
  else
    {
      NFD_LOG_DEBUG("New Entry");
      m_regEntries.insert(RegisteredEntry(
                              parameters.getName(),
                              scheduler::schedule(
                                m_refreshInterval,
                                bind(&RemoteRegistrator::startRegistration,
                                     this, parameters, options, m_nRetries))));
    }
}

void
RemoteRegistrator::onRegFailure(uint32_t code, const std::string& reason,
                                const ControlParameters& parameters,
                                const CommandOptions& options,
                                int nRetries)
{
  NFD_LOG_INFO("fail to register " << parameters.getName()
                                   << "\n\t reason:" << reason
                                   << "\n\t remain retries:" << nRetries);

  if (nRetries > 0)
    {
      startRegistration(parameters, options, nRetries - 1);
    }
}

void
RemoteRegistrator::onUnregSuccess(const ControlParameters& parameters,
                                  const CommandOptions& options)
{
  NFD_LOG_INFO("success to unregister " << parameters.getName());
}

void
RemoteRegistrator::onUnregFailure(uint32_t code, const std::string& reason,
                                  const ControlParameters& parameters,
                                  const CommandOptions& options,
                                  int nRetries)
{
  NFD_LOG_INFO("fail to unregister " << parameters.getName()
                                     << "\n\t reason:" << reason
                                     << "\n\t remain retries:" << nRetries);

  if (nRetries > 0)
    {
      startUnregistration(parameters, options, nRetries - 1);
    }
}

void
RemoteRegistrator::redoRegistration()
{
  NFD_LOG_INFO("redo " << m_regEntries.size()
                       << " registration when new Hub connection is built.");

  for (auto&& entry : m_regEntries)
    {
      // make copies to avoid unreasonable overwrite.
      ControlParameters parameters = m_controlParameters;
      CommandOptions    options    = m_commandOptions;
      startRegistration(parameters.setName(entry.first),
                        options.setSigningInfo(signingByIdentity(entry.first)),
                        m_nRetries);
    }
}

void
RemoteRegistrator::clearRefreshEvents()
{
  for (auto&& entry : m_regEntries)
    {
      scheduler::cancel(entry.second);
    }
}

} // namespace rib
} // namespace nfd
