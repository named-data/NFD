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

#ifndef NFD_DAEMON_RIB_SERVICE_HPP
#define NFD_DAEMON_RIB_SERVICE_HPP

#include "core/config-file.hpp"
#include "mgmt/rib-manager.hpp"
#include "rib/fib-updater.hpp"
#include "rib/rib.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/transport/transport.hpp>
#include <ndn-cxx/util/scheduler.hpp>

namespace nfd {
namespace rib {

class Readvertise;

/**
 * \brief initializes and executes NFD-RIB service thread
 *
 * Only one instance of this class can be created at any time.
 * After initialization, NFD-RIB instance can be started by running the global io_service.
 */
class Service : noncopyable
{
public:
  /**
   * \brief create NFD-RIB service
   * \param configFile absolute or relative path of configuration file
   * \param keyChain the KeyChain
   * \throw std::logic_error Instance of rib::Service has been already constructed
   * \throw std::logic_error Instance of rib::Service is not constructed on RIB thread
   */
  Service(const std::string& configFile, ndn::KeyChain& keyChain);

  /**
   * \brief create NFD-RIB service
   * \param configSection parsed configuration section
   * \param keyChain the KeyChain
   * \note This constructor overload is more appropriate for integrated environments,
   *       such as NS-3 or android. Error messages related to configuration file
   *       will use "internal://nfd.conf" as configuration filename.
   * \throw std::logic_error Instance of rib::Service has been already constructed
   * \throw std::logic_error Instance of rib::Service is not constructed on RIB thread
   */
  Service(const ConfigSection& configSection, ndn::KeyChain& keyChain);

  /**
   * \brief Destructor
   */
  ~Service();

  /**
   * \brief Get a reference to the only instance of this class
   * \throw std::logic_error No instance has been constructed
   * \throw std::logic_error This function is invoked on a thread other than the RIB thread
   */
  static Service&
  get();

  RibManager&
  getRibManager()
  {
    return m_ribManager;
  }

private:
  template<typename ConfigParseFunc>
  Service(ndn::KeyChain& keyChain, shared_ptr<ndn::Transport> localNfdTransport,
          const ConfigParseFunc& configParse);

  void
  processConfig(const ConfigSection& section, bool isDryRun, const std::string& filename);

  void
  checkConfig(const ConfigSection& section, const std::string& filename);

  void
  applyConfig(const ConfigSection& section, const std::string& filename);

private:
  static Service* s_instance;

  ndn::KeyChain& m_keyChain;
  ndn::Face m_face;
  ndn::nfd::Controller m_nfdController;

  Rib m_rib;
  FibUpdater m_fibUpdater;
  unique_ptr<Readvertise> m_readvertiseNlsr;
  unique_ptr<Readvertise> m_readvertisePropagation;
  ndn::mgmt::Dispatcher m_dispatcher;
  RibManager m_ribManager;
};

} // namespace rib
} // namespace nfd

#endif // NFD_DAEMON_RIB_SERVICE_HPP
