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

#include "face-system.hpp"
#include "protocol-factory.hpp"
#include "netdev-bound.hpp"
#include "common/global.hpp"
#include "fw/face-table.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT(FaceSystem);

const std::string CFGSEC_FACESYSTEM = "face_system";
const std::string CFGSEC_GENERAL = "general";
const std::string CFGSEC_GENERAL_FQ = CFGSEC_FACESYSTEM + ".general";
const std::string CFGSEC_NETDEVBOUND = "netdev_bound";

FaceSystem::FaceSystem(FaceTable& faceTable, shared_ptr<ndn::net::NetworkMonitor> netmon)
  : m_faceTable(faceTable)
  , m_netmon(std::move(netmon))
{
  auto pfCtorParams = this->makePFCtorParams();
  for (const auto& id : ProtocolFactory::listRegistered()) {
    NFD_LOG_TRACE("creating factory " << id);
    m_factories[id] = ProtocolFactory::create(id, pfCtorParams);
  }

  m_netdevBound = make_unique<NetdevBound>(pfCtorParams, *this);
}

ProtocolFactoryCtorParams
FaceSystem::makePFCtorParams()
{
  auto addFace = [this] (auto face) { m_faceTable.add(std::move(face)); };
  return {addFace, m_netmon};
}

FaceSystem::~FaceSystem() = default;

std::set<const ProtocolFactory*>
FaceSystem::listProtocolFactories() const
{
  std::set<const ProtocolFactory*> factories;
  for (const auto& p : m_factories) {
    factories.insert(p.second.get());
  }
  return factories;
}

ProtocolFactory*
FaceSystem::getFactoryById(const std::string& id)
{
  auto found = m_factories.find(id);
  return found == m_factories.end() ? nullptr : found->second.get();
}

ProtocolFactory*
FaceSystem::getFactoryByScheme(const std::string& scheme)
{
  auto found = m_factoryByScheme.find(scheme);
  return found == m_factoryByScheme.end() ? nullptr : found->second;
}

bool
FaceSystem::hasFactoryForScheme(const std::string& scheme) const
{
  return m_factoryByScheme.count(scheme) > 0;
}

void
FaceSystem::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler(CFGSEC_FACESYSTEM, bind(&FaceSystem::processConfig, this, _1, _2, _3));
}

void
FaceSystem::processConfig(const ConfigSection& configSection, bool isDryRun, const std::string&)
{
  ConfigContext context;
  context.isDryRun = isDryRun;

  // process general protocol factory config section
  auto generalSection = configSection.get_child_optional(CFGSEC_GENERAL);
  if (generalSection) {
    for (const auto& pair : *generalSection) {
      const std::string& key = pair.first;
      if (key == "enable_congestion_marking") {
        context.generalConfig.wantCongestionMarking = ConfigFile::parseYesNo(pair, CFGSEC_GENERAL_FQ);
      }
      else {
        NDN_THROW(ConfigFile::Error("Unrecognized option " + CFGSEC_GENERAL_FQ + "." + key));
      }
    }
  }

  // process in protocol factories
  for (const auto& pair : m_factories) {
    const std::string& sectionName = pair.first;
    ProtocolFactory* factory = pair.second.get();

    std::set<std::string> oldProvidedSchemes = factory->getProvidedSchemes();
    factory->processConfig(configSection.get_child_optional(sectionName), context);

    if (!isDryRun) {
      for (const std::string& scheme : factory->getProvidedSchemes()) {
        m_factoryByScheme[scheme] = factory;
        if (oldProvidedSchemes.erase(scheme) == 0) {
          NFD_LOG_TRACE("factory " << sectionName <<
                        " provides " << scheme << " FaceUri scheme");
        }
      }
      for (const std::string& scheme : oldProvidedSchemes) {
        m_factoryByScheme.erase(scheme);
        NFD_LOG_TRACE("factory " << sectionName <<
                      " no longer provides " << scheme << " FaceUri scheme");
      }
    }
  }

  // process netdev_bound section, after factories start providing *+dev schemes
  auto netdevBoundSection = configSection.get_child_optional(CFGSEC_NETDEVBOUND);
  m_netdevBound->processConfig(netdevBoundSection, context);

  // process other sections
  std::set<std::string> seenSections;
  for (const auto& pair : configSection) {
    const std::string& sectionName = pair.first;
    // const ConfigSection& subSection = pair.second;

    if (!seenSections.insert(sectionName).second) {
      NDN_THROW(ConfigFile::Error("Duplicate section " + CFGSEC_FACESYSTEM + "." + sectionName));
    }

    if (sectionName == CFGSEC_GENERAL || sectionName == CFGSEC_NETDEVBOUND ||
        m_factories.count(sectionName) > 0) {
      continue;
    }

    NDN_THROW(ConfigFile::Error("Unrecognized option " + CFGSEC_FACESYSTEM + "." + sectionName));
  }
}

} // namespace face
} // namespace nfd
