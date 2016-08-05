/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_RIB_SERVICE_HPP
#define NFD_RIB_SERVICE_HPP

#include "core/common.hpp"
#include "core/config-file.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/transport/transport.hpp>
#include <ndn-cxx/mgmt/dispatcher.hpp>

namespace nfd {
namespace rib {

class RibManager;

/**
 * \brief initializes and executes NFD-RIB service thread
 */
class Service : noncopyable
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

  /**
   * \brief create NFD-RIB service
   * \param configFile absolute or relative path of configuration file
   * \param keyChain the KeyChain
   */
  Service(const std::string& configFile, ndn::KeyChain& keyChain);

  /**
   * \brief create NFD-RIB service
   * \param config parsed configuration section
   * \param keyChain the KeyChain
   * \note This constructor overload is more appropriate for integrated environments,
   *       such as NS-3 or android. Error messages related to configuration file
   *       will use "internal://nfd.conf" as configuration filename.
   */
  Service(const ConfigSection& config, ndn::KeyChain& keyChain);

  /**
   * \brief Destructor
   */
  ~Service();

  /**
   * \brief Perform initialization of NFD-RIB instance
   *
   * After initialization, NFD-RIB instance can be started by running the global io_service
   */
  void
  initialize();

private:
  void
  initializeLogging();

  /**
   * \brief Look into the config file and construct appropriate transport to communicate with NFD
   * If NFD-RIB instance was initialized with config file, INFO format is assumed
   */
  shared_ptr<ndn::Transport>
  getLocalNfdTransport();

private:
  std::string m_configFile;
  ConfigSection m_configSection;

  ndn::KeyChain& m_keyChain;
  unique_ptr<ndn::Face> m_face;
  unique_ptr<ndn::mgmt::Dispatcher> m_dispatcher;
  unique_ptr<RibManager> m_ribManager;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_SERVICE_HPP
