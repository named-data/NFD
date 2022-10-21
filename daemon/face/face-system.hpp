/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_FACE_SYSTEM_HPP
#define NFD_DAEMON_FACE_FACE_SYSTEM_HPP

#include "network-predicate.hpp"
#include "common/config-file.hpp"

#include <ndn-cxx/net/network-address.hpp>
#include <ndn-cxx/net/network-interface.hpp>
#include <ndn-cxx/net/network-monitor.hpp>

namespace nfd {

class FaceTable;

namespace face {

class NetdevBound;
class ProtocolFactory;
struct ProtocolFactoryCtorParams;

/**
 * \brief Entry point of NFD's face system.
 *
 * NFD's face system is organized as a FaceSystem-ProtocolFactory-Channel-Face hierarchy.
 * The FaceSystem class is the entry point of the face system and owns all ProtocolFactory objects.
 */
class FaceSystem : noncopyable
{
public:
  FaceSystem(FaceTable& faceTable, shared_ptr<ndn::net::NetworkMonitor> netmon);

  ~FaceSystem();

  /**
   * \brief Returns all ProtocolFactory objects owned by the face system.
   */
  [[nodiscard]] std::set<const ProtocolFactory*>
  listProtocolFactories() const;

  /** \return ProtocolFactory for the specified registered factory id or nullptr if not found.
   */
  ProtocolFactory*
  getFactoryById(const std::string& id);

  /** \return ProtocolFactory for the specified FaceUri scheme or nullptr if not found.
   */
  ProtocolFactory*
  getFactoryByScheme(const std::string& scheme);

  bool
  hasFactoryForScheme(const std::string& scheme) const;

  FaceTable&
  getFaceTable()
  {
    return m_faceTable;
  }

  /** \brief Register handler for the `face_system` section of NFD's configuration file.
   */
  void
  setConfigFile(ConfigFile& configFile);

  /** \brief Configuration options from `general` section.
   */
  struct GeneralConfig
  {
    bool wantCongestionMarking = true;
  };

  /** \brief Context for processing a config section in ProtocolFactory.
   */
  class ConfigContext : noncopyable
  {
  public:
    GeneralConfig generalConfig;
    bool isDryRun;
  };

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  ProtocolFactoryCtorParams
  makePFCtorParams();

private:
  void
  processConfig(const ConfigSection& configSection, bool isDryRun,
                const std::string& filename);

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /** \brief Config section name => protocol factory.
   */
  std::map<std::string, unique_ptr<ProtocolFactory>> m_factories;
  unique_ptr<NetdevBound> m_netdevBound;

private:
  /** \brief Scheme => protocol factory.
   *
   *  The same protocol factory may be available under multiple schemes.
   */
  std::map<std::string, ProtocolFactory*> m_factoryByScheme;

  FaceTable& m_faceTable;
  shared_ptr<ndn::net::NetworkMonitor> m_netmon;
};

} // namespace face

using face::FaceSystem;

} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_SYSTEM_HPP
