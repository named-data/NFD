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

#include "nfd.hpp"

#include "core/global-io.hpp"
#include "core/logger-factory.hpp"
#include "core/privilege-helper.hpp"
#include "core/config-file.hpp"
#include "fw/forwarder.hpp"
#include "face/null-face.hpp"
#include "mgmt/internal-face.hpp"
#include "mgmt/fib-manager.hpp"
#include "mgmt/face-manager.hpp"
#include "mgmt/strategy-choice-manager.hpp"
#include "mgmt/status-server.hpp"
#include "mgmt/general-config-section.hpp"
#include "mgmt/tables-config-section.hpp"

namespace nfd {

NFD_LOG_INIT("Nfd");

static const std::string INTERNAL_CONFIG = "internal://nfd.conf";

Nfd::Nfd(const std::string& configFile, ndn::KeyChain& keyChain)
  : m_configFile(configFile)
  , m_keyChain(keyChain)
  , m_networkMonitor(getGlobalIoService())
{
}

Nfd::Nfd(const ConfigSection& config, ndn::KeyChain& keyChain)
  : m_configSection(config)
  , m_keyChain(keyChain)
  , m_networkMonitor(getGlobalIoService())
{
}

Nfd::~Nfd()
{
  // It is necessary to explicitly define the destructor, because some member variables (e.g.,
  // unique_ptr<Forwarder>) are forward-declared, but implicitly declared destructor requires
  // complete types for all members when instantiated.
}

void
Nfd::initialize()
{
  initializeLogging();

  m_forwarder.reset(new Forwarder());

  initializeManagement();

  m_forwarder->getFaceTable().addReserved(make_shared<NullFace>(), FACEID_NULL);
  m_forwarder->getFaceTable().addReserved(make_shared<NullFace>(FaceUri("contentstore://")),
                                          FACEID_CONTENT_STORE);

  PrivilegeHelper::drop();

  m_networkMonitor.onNetworkStateChanged.connect([this] {
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
  m_internalFace = make_shared<InternalFace>();

  m_fibManager.reset(new FibManager(m_forwarder->getFib(),
                                    bind(&Forwarder::getFace, m_forwarder.get(), _1),
                                    m_internalFace, m_keyChain));

  m_faceManager.reset(new FaceManager(m_forwarder->getFaceTable(), m_internalFace, m_keyChain));

  m_strategyChoiceManager.reset(new StrategyChoiceManager(m_forwarder->getStrategyChoice(),
                                                          m_internalFace, m_keyChain));

  m_statusServer.reset(new StatusServer(m_internalFace, *m_forwarder, m_keyChain));

  ConfigFile config(&ignoreRibAndLogSections);
  general::setConfigFile(config);

  TablesConfigSection tablesConfig(m_forwarder->getCs(),
                                   m_forwarder->getPit(),
                                   m_forwarder->getFib(),
                                   m_forwarder->getStrategyChoice(),
                                   m_forwarder->getMeasurements());
  tablesConfig.setConfigFile(config);

  m_internalFace->getValidator().setConfigFile(config);

  m_forwarder->getFaceTable().addReserved(m_internalFace, FACEID_INTERNAL_FACE);

  m_faceManager->setConfigFile(config);

  // parse config file
  if (!m_configFile.empty()) {
    config.parse(m_configFile, true);
    config.parse(m_configFile, false);
  }
  else {
    config.parse(m_configSection, true, INTERNAL_CONFIG);
    config.parse(m_configSection, false, INTERNAL_CONFIG);
  }

  tablesConfig.ensureTablesAreConfigured();

  // add FIB entry for NFD Management Protocol
  shared_ptr<fib::Entry> entry = m_forwarder->getFib().insert("/localhost/nfd").first;
  entry->addNextHop(m_internalFace, 0);
}

void
Nfd::reloadConfigFile()
{
  // Logging
  initializeLogging();
  /// \todo Reopen log file

  // Other stuff
  ConfigFile config(&ignoreRibAndLogSections);

  general::setConfigFile(config);

  TablesConfigSection tablesConfig(m_forwarder->getCs(),
                                   m_forwarder->getPit(),
                                   m_forwarder->getFib(),
                                   m_forwarder->getStrategyChoice(),
                                   m_forwarder->getMeasurements());

  tablesConfig.setConfigFile(config);

  m_internalFace->getValidator().setConfigFile(config);
  m_faceManager->setConfigFile(config);

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
  m_faceManager->setConfigFile(config);

  if (!m_configFile.empty()) {
    config.parse(m_configFile, false);
  }
  else {
    config.parse(m_configSection, false, INTERNAL_CONFIG);
  }
}

} // namespace nfd
