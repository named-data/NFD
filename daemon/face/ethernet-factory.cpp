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

#include "ethernet-factory.hpp"
#include "ethernet-transport.hpp"
#include "generic-link-service.hpp"
#include "core/logger.hpp"
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace nfd {
namespace face {

NFD_LOG_INIT("EthernetFactory");
NFD_REGISTER_PROTOCOL_FACTORY(EthernetFactory);

const std::string&
EthernetFactory::getId()
{
  static std::string id("ether");
  return id;
}


void
EthernetFactory::processConfig(OptionalConfigSection configSection,
                               FaceSystem::ConfigContext& context)
{
  // ether
  // {
  //   mcast yes
  //   mcast_group 01:00:5E:00:17:AA
  //   whitelist
  //   {
  //     *
  //   }
  //   blacklist
  //   {
  //   }
  // }

  MulticastConfig mcastConfig;

  if (configSection) {
    // face_system.ether.mcast defaults to 'yes' but only if face_system.ether section is present
    mcastConfig.isEnabled = true;

    for (const auto& pair : *configSection) {
      const std::string& key = pair.first;
      const ConfigSection& value = pair.second;

      if (key == "mcast") {
        mcastConfig.isEnabled = ConfigFile::parseYesNo(pair, "ether");
      }
      else if (key == "mcast_group") {
        const std::string& valueStr = value.get_value<std::string>();
        mcastConfig.group = ethernet::Address::fromString(valueStr);
        if (mcastConfig.group.isNull()) {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("face_system.ether.mcast_group: '" +
                                valueStr + "' cannot be parsed as an Ethernet address"));
        }
        else if (!mcastConfig.group.isMulticast()) {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("face_system.ether.mcast_group: '" +
                                valueStr + "' is not a multicast address"));
        }
      }
      else if (key == "whitelist") {
        mcastConfig.netifPredicate.parseWhitelist(value);
      }
      else if (key == "blacklist") {
        mcastConfig.netifPredicate.parseBlacklist(value);
      }
      else {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option face_system.ether." + key));
      }
    }
  }

  if (!context.isDryRun) {
    if (m_mcastConfig.isEnabled != mcastConfig.isEnabled) {
      if (mcastConfig.isEnabled) {
        NFD_LOG_INFO("enabling multicast on " << mcastConfig.group);
      }
      else {
        NFD_LOG_INFO("disabling multicast");
      }
    }
    else if (m_mcastConfig.group != mcastConfig.group) {
      NFD_LOG_INFO("changing multicast group from " << m_mcastConfig.group <<
                   " to " << mcastConfig.group);
    }
    else if (m_mcastConfig.netifPredicate != mcastConfig.netifPredicate) {
      NFD_LOG_INFO("changing whitelist/blacklist");
    }
    else {
      // There's no configuration change, but we still need to re-apply configuration because
      // netifs may have changed.
    }

    m_mcastConfig = mcastConfig;
    this->applyConfig(context);
  }
}

void
EthernetFactory::createFace(const FaceUri& uri,
                            ndn::nfd::FacePersistency persistency,
                            bool wantLocalFieldsEnabled,
                            const FaceCreatedCallback& onCreated,
                            const FaceCreationFailedCallback& onFailure)
{
  onFailure(406, "Unsupported protocol");
}

std::vector<shared_ptr<const Channel>>
EthernetFactory::getChannels() const
{
  return {};
}

shared_ptr<Face>
EthernetFactory::createMulticastFace(const NetworkInterfaceInfo& netif,
                                     const ethernet::Address& address)
{
  BOOST_ASSERT(address.isMulticast());

  auto key = std::make_pair(netif.name, address);
  auto found = m_mcastFaces.find(key);
  if (found != m_mcastFaces.end()) {
    return found->second;
  }

  face::GenericLinkService::Options opts;
  opts.allowFragmentation = true;
  opts.allowReassembly = true;

  auto linkService = make_unique<face::GenericLinkService>(opts);
  auto transport = make_unique<face::EthernetTransport>(netif, address);
  auto face = make_shared<Face>(std::move(linkService), std::move(transport));

  m_mcastFaces[key] = face;
  connectFaceClosedSignal(*face, [this, key] { m_mcastFaces.erase(key); });

  return face;
}

void
EthernetFactory::applyConfig(const FaceSystem::ConfigContext& context)
{
  // collect old faces
  std::set<shared_ptr<Face>> oldFaces;
  boost::copy(m_mcastFaces | boost::adaptors::map_values,
              std::inserter(oldFaces, oldFaces.end()));

  if (m_mcastConfig.isEnabled) {
    // determine interfaces on which faces should be created or retained
    auto capableNetifs = context.listNetifs() |
                         boost::adaptors::filtered([this] (const NetworkInterfaceInfo& netif) {
                           return netif.isUp() && netif.isMulticastCapable() &&
                                  m_mcastConfig.netifPredicate(netif);
                         });

    // create faces
    for (const auto& netif : capableNetifs) {
      shared_ptr<Face> face;
      try {
        face = this->createMulticastFace(netif, m_mcastConfig.group);
      }
      catch (const EthernetTransport::Error& e) {
        NFD_LOG_ERROR("Cannot create Ethernet multicast face on " << netif.name << ": " <<
                      e.what() << ", continuing");
        continue;
      }

      if (face->getId() == face::INVALID_FACEID) {
        // new face: register with forwarding
        context.addFace(face);
      }
      else {
        // existing face: don't destroy
        oldFaces.erase(face);
      }
    }
  }

  // destroy old faces that are not needed in new configuration
  for (const auto& face : oldFaces) {
    face->close();
  }
}

} // namespace face
} // namespace nfd
