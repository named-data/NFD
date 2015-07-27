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

#include "nrd.hpp"

#include "rib-manager.hpp"
#include "core/config-file.hpp"
#include "core/logger-factory.hpp"
#include "core/global-io.hpp"

#include <boost/property_tree/info_parser.hpp>

#include <ndn-cxx/transport/unix-transport.hpp>
#include <ndn-cxx/transport/tcp-transport.hpp>

namespace nfd {
namespace rib {

static const std::string INTERNAL_CONFIG = "internal://nfd.conf";

Nrd::Nrd(const std::string& configFile, ndn::KeyChain& keyChain)
  : m_configFile(configFile)
  , m_keyChain(keyChain)
{
}

Nrd::Nrd(const ConfigSection& config, ndn::KeyChain& keyChain)
  : m_configSection(config)
  , m_keyChain(keyChain)
{
}

Nrd::~Nrd()
{
  // It is necessary to explicitly define the destructor, because some member variables
  // (e.g., unique_ptr<RibManager>) are forward-declared, but implicitly declared destructor
  // requires complete types for all members when instantiated.
}

void
Nrd::initialize()
{
  m_face.reset(new ndn::Face(getLocalNfdTransport(), getGlobalIoService(), m_keyChain));

  initializeLogging();

  m_ribManager.reset(new RibManager(*m_face, m_keyChain));

  ConfigFile config([] (const std::string& filename, const std::string& sectionName,
                        const ConfigSection& section, bool isDryRun) {
      // Ignore "log" and sections belonging to NFD,
      // but raise an error if we're missing a handler for a "rib" section.
      if (sectionName != "rib" || sectionName == "log") {
        // do nothing
      }
      else {
        // missing NRD section
        ConfigFile::throwErrorOnUnknownSection(filename, sectionName, section, isDryRun);
      }
    });
  m_ribManager->setConfigFile(config);

  // parse config file
  if (!m_configFile.empty()) {
    config.parse(m_configFile, true);
    config.parse(m_configFile, false);
  }
  else {
    config.parse(m_configSection, true, INTERNAL_CONFIG);
    config.parse(m_configSection, false, INTERNAL_CONFIG);
  }

  m_ribManager->registerWithNfd();
  m_ribManager->enableLocalControlHeader();
}

void
Nrd::initializeLogging()
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

shared_ptr<ndn::Transport>
Nrd::getLocalNfdTransport()
{
  ConfigSection config;

  if (!m_configFile.empty()) {
    // Any format errors should have been caught already
    // If error is thrown at this point, it is development error
    boost::property_tree::read_info(m_configFile, config);
  }
  else
    config = m_configSection;

  if (config.get_child_optional("face_system.unix")) {
    // unix socket enabled

    auto&& socketPath = config.get<std::string>("face_system.unix.path", "/var/run/nfd.sock");
    // default socketPath should be the same as in FaceManager::processSectionUnix

    return make_shared<ndn::UnixTransport>(socketPath);
  }
  else if (config.get_child_optional("face_system.tcp") &&
           config.get<std::string>("face_system.tcp.listen", "yes") == "yes") {
    // tcp is enabled

    auto&& port = config.get<std::string>("face_system.tcp.port", "6363");
    // default port should be the same as in FaceManager::processSectionTcp

    return make_shared<ndn::TcpTransport>("localhost", port);
  }
  else {
    BOOST_THROW_EXCEPTION(Error("No transport is available to communicate with NFD"));
  }
}

} // namespace rib
} // namespace nfd
