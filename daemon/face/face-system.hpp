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

#ifndef NFD_DAEMON_FACE_FACE_SYSTEM_HPP
#define NFD_DAEMON_FACE_FACE_SYSTEM_HPP

#include "core/config-file.hpp"
#include "protocol-factory.hpp"

namespace nfd {

class FaceTable;
class NetworkInterfaceInfo;

namespace face {

/** \brief entry point of the face system
 *
 *  FaceSystem class is the entry point of NFD's face system.
 *  It owns ProtocolFactory objects that are created from face_system section of NFD configuration file.
 */
class FaceSystem : noncopyable
{
public:
  explicit
  FaceSystem(FaceTable& faceTable);

  /** \return ProtocolFactory objects owned by the FaceSystem
   */
  std::set<const ProtocolFactory*>
  listProtocolFactories() const;

  /** \return ProtocolFactory for specified protocol scheme, or nullptr if not found
   */
  ProtocolFactory*
  getProtocolFactory(const std::string& scheme);

  /** \brief register handler for face_system section of NFD configuration file
   */
  void
  setConfigFile(ConfigFile& configFile);

private:
  void
  processConfig(const ConfigSection& configSection, bool isDryRun,
                const std::string& filename);

  void
  processSectionUnix(const ConfigSection& configSection, bool isDryRun);

  void
  processSectionTcp(const ConfigSection& configSection, bool isDryRun);

  void
  processSectionUdp(const ConfigSection& configSection, bool isDryRun,
                    const std::vector<NetworkInterfaceInfo>& nicList);

  void
  processSectionEther(const ConfigSection& configSection, bool isDryRun,
                      const std::vector<NetworkInterfaceInfo>& nicList);

  void
  processSectionWebSocket(const ConfigSection& configSection, bool isDryRun);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /** \brief scheme => protocol factory
   *
   *  The same protocol factory may be available under multiple schemes.
   */
  std::map<std::string, shared_ptr<ProtocolFactory>> m_factories;

  FaceTable& m_faceTable;
};

} // namespace face

using face::FaceSystem;

} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_SYSTEM_HPP
