/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "nfd.hpp"

#include "core/config-file.hpp"
#include "core/global-io.hpp"
#include "core/logger-factory.hpp"
#include "core/privilege-helper.hpp"
#include "face/face-system.hpp"
#include "face/internal-face.hpp"
#include "face/null-face.hpp"
#include "fw/forwarder.hpp"
#include "mgmt/cs-manager.hpp"
#include "mgmt/face-manager.hpp"
#include "mgmt/fib-manager.hpp"
#include "mgmt/forwarder-status-manager.hpp"
#include "mgmt/general-config-section.hpp"
#include "mgmt/strategy-choice-manager.hpp"
#include "mgmt/tables-config-section.hpp"

namespace nfd {

NFD_LOG_INIT("Nfd");

static const std::string INTERNAL_CONFIG = "internal://nfd.conf";

Nfd::Nfd(ndn::KeyChain& keyChain)
  : m_keyChain(keyChain)
  , m_netmon(make_shared<ndn::net::NetworkMonitor>(getGlobalIoService()))
{
}

Nfd::Nfd(const std::string& configFile, ndn::KeyChain& keyChain)
  : Nfd(keyChain)
{
  m_configFile = configFile;
}

Nfd::Nfd(const ConfigSection& config, ndn::KeyChain& keyChain)
  : Nfd(keyChain)
{
  m_configSection = config;
}

// It is necessary to explicitly define the destructor, because some member variables (e.g.,
// unique_ptr<Forwarder>) are forward-declared, but implicitly declared destructor requires
// complete types for all members when instantiated.
Nfd::~Nfd() = default;

void
Nfd::initialize()
{
  initializeLogging();

  m_forwarder = make_unique<Forwarder>();

  FaceTable& faceTable = m_forwarder->getFaceTable();
  faceTable.addReserved(face::makeNullFace(), face::FACEID_NULL);
  faceTable.addReserved(face::makeNullFace(FaceUri("contentstore://")), face::FACEID_CONTENT_STORE);
  m_faceSystem = make_unique<face::FaceSystem>(faceTable, m_netmon);

  initializeManagement();

  PrivilegeHelper::drop();

  m_netmon->onNetworkStateChanged.connect([this] {
      // delay stages, so if multiple events are triggered in short sequence,
      // only one auto-detection procedure is triggered
      m_reloadConfigEvent = scheduler::schedule(time::seconds(5),
        [this] {
          NFD_LOG_INFO("Network change detected, reloading face section of the config file...");
          this->reloadConfigFileFaceSection();
        });
    });
}

void
Nfd::initializeLogging()
{
  ConfigFile config(&ConfigFile::ignoreUnknownSection);
  LoggerFactory::getInstance().setConfigFile(config);

  if (!m_configFile.empty()) {
    config.parse(m_configFile, true);
    config.parse(m_configFile, false);
  }
  else {
    config.parse(m_configSection, true, INTERNAL_CONFIG);
    config.parse(m_configSection, false, INTERNAL_CONFIG);
  }
}

static inline void
ignoreRibAndLogSections(const std::string& filename, const std::string& sectionName,
                        const ConfigSection& section, bool isDryRun)
{
  // Ignore "log" and "rib" sections, but raise an error if we're missing a
  // handler for an NFD section.
  if (sectionName == "rib" || sectionName == "log") {
    // do nothing
  }
  else {
    // missing NFD section
    ConfigFile::throwErrorOnUnknownSection(filename, sectionName, section, isDryRun);
  }
}

void
Nfd::initializeManagement()
{
  std::tie(m_internalFace, m_internalClientFace) = face::makeInternalFace(m_keyChain);
  m_forwarder->getFaceTable().addReserved(m_internalFace, face::FACEID_INTERNAL_FACE);

  m_dispatcher = make_unique<ndn::mgmt::Dispatcher>(*m_internalClientFace, m_keyChain);
  m_authenticator = CommandAuthenticator::create();

  m_forwarderStatusManager = make_unique<ForwarderStatusManager>(*m_forwarder, *m_dispatcher);
  m_faceManager = make_unique<FaceManager>(*m_faceSystem, *m_dispatcher, *m_authenticator);
  m_fibManager = make_unique<FibManager>(m_forwarder->getFib(), m_forwarder->getFaceTable(),
                                         *m_dispatcher, *m_authenticator);
  m_csManager = make_unique<CsManager>(m_forwarder->getCs(), m_forwarder->getCounters(),
                                       *m_dispatcher, *m_authenticator);
  m_strategyChoiceManager = make_unique<StrategyChoiceManager>(m_forwarder->getStrategyChoice(),
                                                               *m_dispatcher, *m_authenticator);

  ConfigFile config(&ignoreRibAndLogSections);
  general::setConfigFile(config);

  TablesConfigSection tablesConfig(*m_forwarder);
  tablesConfig.setConfigFile(config);

  m_authenticator->setConfigFile(config);
  m_faceSystem->setConfigFile(config);

  // parse config file
  if (!m_configFile.empty()) {
    config.parse(m_configFile, true);
    config.parse(m_configFile, false);
  }
  else {
    config.parse(m_configSection, true, INTERNAL_CONFIG);
    config.parse(m_configSection, false, INTERNAL_CONFIG);
  }

  tablesConfig.ensureConfigured();

  // add FIB entry for NFD Management Protocol
  Name topPrefix("/localhost/nfd");
  m_forwarder->getFib().insert(topPrefix).first->addNextHop(*m_internalFace, 0);
  m_dispatcher->addTopPrefix(topPrefix, false);
}

void
Nfd::reloadConfigFile()
{
  // Logging
  initializeLogging();

  // Other stuff
  ConfigFile config(&ignoreRibAndLogSections);

  general::setConfigFile(config);

  TablesConfigSection tablesConfig(*m_forwarder);
  tablesConfig.setConfigFile(config);

  m_authenticator->setConfigFile(config);
  m_faceSystem->setConfigFile(config);

  if (!m_configFile.empty()) {
    config.parse(m_configFile, false);
  }
  else {
    config.parse(m_configSection, false, INTERNAL_CONFIG);
  }
}

void
Nfd::reloadConfigFileFaceSection()
{
  // reload only face_system section of the config file to re-initialize multicast faces
  ConfigFile config(&ConfigFile::ignoreUnknownSection);
  m_faceSystem->setConfigFile(config);

  if (!m_configFile.empty()) {
    config.parse(m_configFile, false);
  }
  else {
    config.parse(m_configSection, false, INTERNAL_CONFIG);
  }
}

} // namespace nfd
