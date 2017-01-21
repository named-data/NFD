/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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
// #include "core/logger.hpp"
#include "fw/face-table.hpp"

// ProtocolFactory includes, sorted alphabetically
#ifdef HAVE_LIBPCAP
#include "ethernet-factory.hpp"
#endif // HAVE_LIBPCAP
#include "tcp-factory.hpp"
#include "udp-factory.hpp"
#ifdef HAVE_UNIX_SOCKETS
#include "unix-stream-factory.hpp"
#endif // HAVE_UNIX_SOCKETS
#ifdef HAVE_WEBSOCKET
#include "websocket-factory.hpp"
#endif // HAVE_WEBSOCKET

namespace nfd {
namespace face {

// NFD_LOG_INIT("FaceSystem");

FaceSystem::FaceSystem(FaceTable& faceTable)
  : m_faceTable(faceTable)
{
  ///\todo #3904 make a registry, and construct instances from registry

#ifdef HAVE_LIBPCAP
  m_factories["ether"] = make_shared<EthernetFactory>();
#endif // HAVE_LIBPCAP

  m_factories["tcp"] = make_shared<TcpFactory>();

  m_factories["udp"] = make_shared<UdpFactory>();

#ifdef HAVE_UNIX_SOCKETS
  m_factories["unix"] = make_shared<UnixStreamFactory>();
#endif // HAVE_UNIX_SOCKETS

#ifdef HAVE_WEBSOCKET
  m_factories["websocket"] = make_shared<WebSocketFactory>();
#endif // HAVE_WEBSOCKET
}

std::set<const ProtocolFactory*>
FaceSystem::listProtocolFactories() const
{
  std::set<const ProtocolFactory*> factories;
  for (const auto& p : m_factoryByScheme) {
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
  return found == m_factoryByScheme.end() ? nullptr : found->second.get();
}

void
FaceSystem::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("face_system", bind(&FaceSystem::processConfig, this, _1, _2, _3));
}

void
FaceSystem::processConfig(const ConfigSection& configSection, bool isDryRun, const std::string& filename)
{
  ConfigContext context;
  context.isDryRun = isDryRun;
  context.addFace = bind(&FaceTable::add, &m_faceTable, _1);
  context.m_netifs = listNetworkInterfaces();

  // process sections in protocol factories
  for (const auto& pair : m_factories) {
    const std::string& sectionName = pair.first;
    shared_ptr<ProtocolFactory> factory = pair.second;

    std::set<std::string> oldProvidedSchemes = factory->getProvidedSchemes();
    factory->processConfig(configSection.get_child_optional(sectionName), context);

    if (!isDryRun) {
      for (const std::string& scheme : factory->getProvidedSchemes()) {
        m_factoryByScheme[scheme] = factory;
        oldProvidedSchemes.erase(scheme);
      }
      for (const std::string& scheme : oldProvidedSchemes) {
        m_factoryByScheme.erase(scheme);
      }
    }
  }

  // process other sections
  std::set<std::string> seenSections;
  for (const auto& pair : configSection) {
    const std::string& sectionName = pair.first;
    // const ConfigSection& subSection = pair.second;

    if (!seenSections.insert(sectionName).second) {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Duplicate section face_system." + sectionName));
    }

    if (m_factories.count(sectionName) > 0) {
      continue;
    }

    ///\todo #3521 nicfaces

    BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option face_system." + sectionName));
  }
}

} // namespace face
} // namespace nfd
