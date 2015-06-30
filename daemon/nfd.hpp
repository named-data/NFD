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

#ifndef NFD_DAEMON_NFD_HPP
#define NFD_DAEMON_NFD_HPP

#include "common.hpp"
#include "core/config-file.hpp"
#include "core/scheduler.hpp"

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/network-monitor.hpp>

namespace nfd {

class Forwarder;
class InternalFace;
class FibManager;
class FaceManager;
class StrategyChoiceManager;
class StatusServer;

/**
 * \brief Class representing NFD instance
 * This class can be used to initialize all components of NFD
 */
class Nfd : noncopyable
{
public:
  /**
   * \brief Create NFD instance using absolute or relative path to \p configFile
   */
  Nfd(const std::string& configFile, ndn::KeyChain& keyChain);

  /**
   * \brief Create NFD instance using a parsed ConfigSection \p config
   * This version of the constructor is more appropriate for integrated environments,
   * such as NS-3 or android.
   * \note When using this version of the constructor, error messages will include
   *      "internal://nfd.conf" when referring to configuration errors.
   */
  Nfd(const ConfigSection& config, ndn::KeyChain& keyChain);

  /**
   * \brief Destructor
   */
  ~Nfd();

  /**
   * \brief Perform initialization of NFD instance
   * After initialization, NFD instance can be started by invoking run on globalIoService
   */
  void
  initialize();

  /**
   * \brief Reload configuration file and apply update (if any)
   */
  void
  reloadConfigFile();

private:
  void
  initializeLogging();

  void
  initializeManagement();

  void
  reloadConfigFileFaceSection();

private:
  std::string m_configFile;
  ConfigSection m_configSection;

  unique_ptr<Forwarder> m_forwarder;

  shared_ptr<InternalFace>          m_internalFace;
  unique_ptr<FibManager>            m_fibManager;
  unique_ptr<FaceManager>           m_faceManager;
  unique_ptr<StrategyChoiceManager> m_strategyChoiceManager;
  unique_ptr<StatusServer>          m_statusServer;

  ndn::KeyChain&                    m_keyChain;

  ndn::util::NetworkMonitor         m_networkMonitor;
  scheduler::ScopedEventId          m_reloadConfigEvent;
};

} // namespace nfd

#endif // NFD_DAEMON_NFD_HPP
