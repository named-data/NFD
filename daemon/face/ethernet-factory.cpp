/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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
#include "generic-link-service.hpp"
#include "multicast-ethernet-transport.hpp"

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

EthernetFactory::EthernetFactory(const CtorParams& params)
  : ProtocolFactory(params)
{
  m_netifAddConn = netmon->onInterfaceAdded.connect(
    [this] (const shared_ptr<const ndn::net::NetworkInterface>& netif) {
      this->applyUnicastConfigToNetif(netif);
      this->applyMcastConfigToNetif(*netif);
    });
}

void
EthernetFactory::processConfig(OptionalConfigSection configSection,
                               FaceSystem::ConfigContext& context)
{
  // ether
  // {
  //   listen yes
  //   idle_timeout 600
  //   mcast yes
  //   mcast_group 01:00:5E:00:17:AA
  //   mcast_ad_hoc no
  //   whitelist
  //   {
  //     *
  //   }
  //   blacklist
  //   {
  //   }
  // }

  UnicastConfig unicastConfig;
  MulticastConfig mcastConfig;

  if (configSection) {
    // listen and mcast default to 'yes' but only if face_system.ether section is present
    unicastConfig.isEnabled = unicastConfig.wantListen = mcastConfig.isEnabled = true;

    for (const auto& pair : *configSection) {
      const std::string& key = pair.first;
      const ConfigSection& value = pair.second;

      if (key == "listen") {
        unicastConfig.wantListen = ConfigFile::parseYesNo(pair, "face_system.ether");
      }
      else if (key == "idle_timeout") {
        unicastConfig.idleTimeout = time::seconds(ConfigFile::parseNumber<uint32_t>(pair, "face_system.ether"));
      }
      else if (key == "mcast") {
        mcastConfig.isEnabled = ConfigFile::parseYesNo(pair, "face_system.ether");
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
      else if (key == "mcast_ad_hoc") {
        bool wantAdHoc = ConfigFile::parseYesNo(pair, "face_system.ether");
        mcastConfig.linkType = wantAdHoc ? ndn::nfd::LINK_TYPE_AD_HOC : ndn::nfd::LINK_TYPE_MULTI_ACCESS;
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

  if (context.isDryRun) {
    return;
  }

  if (unicastConfig.isEnabled) {
    if (m_unicastConfig.wantListen && !unicastConfig.wantListen && !m_channels.empty()) {
      NFD_LOG_WARN("Cannot stop listening on Ethernet channels");
    }
    if (m_unicastConfig.idleTimeout != unicastConfig.idleTimeout && !m_channels.empty()) {
      NFD_LOG_WARN("Idle timeout setting applies to new Ethernet channels only");
    }
  }
  else if (m_unicastConfig.isEnabled && !m_channels.empty()) {
    NFD_LOG_WARN("Cannot disable Ethernet channels after initialization");
  }

  if (m_mcastConfig.isEnabled != mcastConfig.isEnabled) {
    if (mcastConfig.isEnabled) {
      NFD_LOG_INFO("enabling multicast on " << mcastConfig.group);
    }
    else {
      NFD_LOG_INFO("disabling multicast");
    }
  }
  else if (mcastConfig.isEnabled) {
    if (m_mcastConfig.linkType != mcastConfig.linkType && !m_mcastFaces.empty()) {
      NFD_LOG_WARN("Cannot change ad hoc setting on existing faces");
    }
    if (m_mcastConfig.group != mcastConfig.group) {
      NFD_LOG_INFO("changing multicast group from " << m_mcastConfig.group <<
                   " to " << mcastConfig.group);
    }
    if (m_mcastConfig.netifPredicate != mcastConfig.netifPredicate) {
      NFD_LOG_INFO("changing whitelist/blacklist");
    }
  }

  // Even if there's no configuration change, we still need to re-apply configuration because
  // netifs may have changed.
  m_unicastConfig = unicastConfig;
  m_mcastConfig = mcastConfig;
  this->applyConfig(context);
}

void
EthernetFactory::createFace(const CreateFaceRequest& req,
                            const FaceCreatedCallback& onCreated,
                            const FaceCreationFailedCallback& onFailure)
{
  BOOST_ASSERT(req.remoteUri.isCanonical());

  if (!req.localUri || req.localUri->getScheme() != "dev") {
    NFD_LOG_TRACE("Cannot create unicast Ethernet face without dev:// LocalUri");
    onFailure(406, "Creation of unicast Ethernet faces requires a LocalUri with dev:// scheme");
    return;
  }
  BOOST_ASSERT(req.localUri->isCanonical());

  if (req.params.persistency == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND) {
    NFD_LOG_TRACE("createFace does not support FACE_PERSISTENCY_ON_DEMAND");
    onFailure(406, "Outgoing Ethernet faces do not support on-demand persistency");
    return;
  }

  ethernet::Address remoteEndpoint(ethernet::Address::fromString(req.remoteUri.getHost()));
  std::string localEndpoint(req.localUri->getHost());

  if (remoteEndpoint.isMulticast()) {
    NFD_LOG_TRACE("createFace does not support multicast faces");
    onFailure(406, "Cannot create multicast Ethernet faces");
    return;
  }

  if (req.params.wantLocalFields) {
    // Ethernet faces are never local
    NFD_LOG_TRACE("createFace cannot create non-local face with local fields enabled");
    onFailure(406, "Local fields can only be enabled on faces with local scope");
    return;
  }

  for (const auto& i : m_channels) {
    if (i.first == localEndpoint) {
      i.second->connect(remoteEndpoint, req.params, onCreated, onFailure);
      return;
    }
  }

  NFD_LOG_TRACE("No channels available to connect to " << remoteEndpoint);
  onFailure(504, "No channels available to connect");
}

shared_ptr<EthernetChannel>
EthernetFactory::createChannel(const shared_ptr<const ndn::net::NetworkInterface>& localEndpoint,
                               time::nanoseconds idleTimeout)
{
  auto it = m_channels.find(localEndpoint->getName());
  if (it != m_channels.end())
    return it->second;

  auto channel = std::make_shared<EthernetChannel>(localEndpoint, idleTimeout);
  m_channels[localEndpoint->getName()] = channel;

  return channel;
}

std::vector<shared_ptr<const Channel>>
EthernetFactory::getChannels() const
{
  return getChannelsFromMap(m_channels);
}

shared_ptr<Face>
EthernetFactory::createMulticastFace(const ndn::net::NetworkInterface& netif,
                                     const ethernet::Address& address)
{
  BOOST_ASSERT(address.isMulticast());

  auto key = std::make_pair(netif.getName(), address);
  auto found = m_mcastFaces.find(key);
  if (found != m_mcastFaces.end()) {
    return found->second;
  }

  GenericLinkService::Options opts;
  opts.allowFragmentation = true;
  opts.allowReassembly = true;

  auto linkService = make_unique<GenericLinkService>(opts);
  auto transport = make_unique<MulticastEthernetTransport>(netif, address, m_mcastConfig.linkType);
  auto face = make_shared<Face>(std::move(linkService), std::move(transport));

  m_mcastFaces[key] = face;
  connectFaceClosedSignal(*face, [this, key] { m_mcastFaces.erase(key); });

  return face;
}

shared_ptr<EthernetChannel>
EthernetFactory::applyUnicastConfigToNetif(const shared_ptr<const ndn::net::NetworkInterface>& netif)
{
  if (!m_unicastConfig.isEnabled) {
    return nullptr;
  }

  if (netif->getType() != ndn::net::InterfaceType::ETHERNET) {
    NFD_LOG_DEBUG("Not creating channel on " << netif->getName() << ": incompatible netif type");
    return nullptr;
  }

  if (!netif->isUp()) {
    NFD_LOG_DEBUG("Not creating channel on " << netif->getName() << ": netif is down");
    return nullptr;
  }

  if (netif->getEthernetAddress().isNull()) {
    NFD_LOG_DEBUG("Not creating channel on " << netif->getName() << ": invalid Ethernet address");
    return nullptr;
  }

  auto channel = this->createChannel(netif, m_unicastConfig.idleTimeout);
  if (m_unicastConfig.wantListen && !channel->isListening()) {
    try {
      channel->listen(this->addFace, nullptr);
    }
    catch (const EthernetChannel::Error& e) {
      NFD_LOG_WARN("Cannot listen on " << netif->getName() << ": " << e.what());
    }
  }
  return channel;
}

shared_ptr<Face>
EthernetFactory::applyMcastConfigToNetif(const ndn::net::NetworkInterface& netif)
{
  if (!m_mcastConfig.isEnabled) {
    return nullptr;
  }

  if (netif.getType() != ndn::net::InterfaceType::ETHERNET) {
    NFD_LOG_DEBUG("Not creating multicast face on " << netif.getName() << ": incompatible netif type");
    return nullptr;
  }

  if (!netif.isUp()) {
    NFD_LOG_DEBUG("Not creating multicast face on " << netif.getName() << ": netif is down");
    return nullptr;
  }

  if (!netif.canMulticast()) {
    NFD_LOG_DEBUG("Not creating multicast face on " << netif.getName() << ": netif cannot multicast");
    return nullptr;
  }

  if (netif.getEthernetAddress().isNull()) {
    NFD_LOG_DEBUG("Not creating multicast face on " << netif.getName() << ": invalid Ethernet address");
    return nullptr;
  }

  if (!m_mcastConfig.netifPredicate(netif)) {
    NFD_LOG_DEBUG("Not creating multicast face on " << netif.getName() << ": rejected by whitelist/blacklist");
    return nullptr;
  }

  NFD_LOG_DEBUG("Creating multicast face on " << netif.getName());
  shared_ptr<Face> face;
  try {
    face = this->createMulticastFace(netif, m_mcastConfig.group);
  }
  catch (const EthernetTransport::Error& e) {
    NFD_LOG_WARN("Cannot create multicast face on " << netif.getName() << ": " << e.what());
    return nullptr;
  }

  if (face->getId() == face::INVALID_FACEID) {
    // new face: register with forwarding
    this->addFace(face);
  }
  return face;
}

void
EthernetFactory::applyConfig(const FaceSystem::ConfigContext& context)
{
  if (m_unicastConfig.isEnabled) {
    providedSchemes.insert("ether");
  }
  else {
    providedSchemes.erase("ether");
  }

  // collect old multicast faces
  std::set<shared_ptr<Face>> oldFaces;
  boost::copy(m_mcastFaces | boost::adaptors::map_values, std::inserter(oldFaces, oldFaces.end()));

  // create channels and multicast faces if requested by config
  for (const auto& netif : netmon->listNetworkInterfaces()) {
    this->applyUnicastConfigToNetif(netif);

    auto face = this->applyMcastConfigToNetif(*netif);
    if (face != nullptr) {
      // don't destroy face
      oldFaces.erase(face);
    }
  }

  // destroy old multicast faces that are not needed in new configuration
  for (const auto& face : oldFaces) {
    face->close();
  }
}

} // namespace face
} // namespace nfd
