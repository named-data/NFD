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

#include "udp-factory.hpp"
#include "generic-link-service.hpp"
#include "multicast-udp-transport.hpp"
#include "core/global-io.hpp"

#include <ndn-cxx/net/address-converter.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace nfd {
namespace face {

namespace ip = boost::asio::ip;
namespace net = ndn::net;

NFD_LOG_INIT("UdpFactory");
NFD_REGISTER_PROTOCOL_FACTORY(UdpFactory);

const std::string&
UdpFactory::getId()
{
  static std::string id("udp");
  return id;
}

UdpFactory::UdpFactory(const CtorParams& params)
  : ProtocolFactory(params)
{
  m_netifAddConn = netmon->onInterfaceAdded.connect(bind(&UdpFactory::applyMcastConfigToNetif, this, _1));
}

void
UdpFactory::processConfig(OptionalConfigSection configSection,
                          FaceSystem::ConfigContext& context)
{
  // udp
  // {
  //   listen yes
  //   port 6363
  //   enable_v4 yes
  //   enable_v6 yes
  //   idle_timeout 600
  //   mcast yes
  //   mcast_group 224.0.23.170
  //   mcast_port 56363
  //   mcast_group_v6 ff02::1234
  //   mcast_port_v6 56363
  //   mcast_ad_hoc no
  //   whitelist
  //   {
  //     *
  //   }
  //   blacklist
  //   {
  //   }
  // }

  m_wantCongestionMarking = context.generalConfig.wantCongestionMarking;

  bool wantListen = true;
  uint16_t port = 6363;
  bool enableV4 = false;
  bool enableV6 = false;
  uint32_t idleTimeout = 600;
  MulticastConfig mcastConfig;

  if (configSection) {
    // These default to 'yes' but only if face_system.udp section is present
    enableV4 = enableV6 = mcastConfig.isEnabled = true;

    for (const auto& pair : *configSection) {
      const std::string& key = pair.first;
      const ConfigSection& value = pair.second;

      if (key == "listen") {
        wantListen = ConfigFile::parseYesNo(pair, "face_system.udp");
      }
      else if (key == "port") {
        port = ConfigFile::parseNumber<uint16_t>(pair, "face_system.udp");
      }
      else if (key == "enable_v4") {
        enableV4 = ConfigFile::parseYesNo(pair, "face_system.udp");
      }
      else if (key == "enable_v6") {
        enableV6 = ConfigFile::parseYesNo(pair, "face_system.udp");
      }
      else if (key == "idle_timeout") {
        idleTimeout = ConfigFile::parseNumber<uint32_t>(pair, "face_system.udp");
      }
      else if (key == "keep_alive_interval") {
        // ignored
      }
      else if (key == "mcast") {
        mcastConfig.isEnabled = ConfigFile::parseYesNo(pair, "face_system.udp");
      }
      else if (key == "mcast_group") {
        const std::string& valueStr = value.get_value<std::string>();
        boost::system::error_code ec;
        mcastConfig.group.address(ip::address_v4::from_string(valueStr, ec));
        if (ec) {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("face_system.udp.mcast_group: '" +
                                valueStr + "' cannot be parsed as an IPv4 address"));
        }
        else if (!mcastConfig.group.address().is_multicast()) {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("face_system.udp.mcast_group: '" +
                                valueStr + "' is not a multicast address"));
        }
      }
      else if (key == "mcast_port") {
        mcastConfig.group.port(ConfigFile::parseNumber<uint16_t>(pair, "face_system.udp"));
      }
      else if (key == "mcast_group_v6") {
        const std::string& valueStr = value.get_value<std::string>();
        boost::system::error_code ec;
        mcastConfig.groupV6.address(ndn::ip::addressV6FromString(valueStr, ec));
        if (ec) {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("face_system.udp.mcast_group_v6: '" +
                                valueStr + "' cannot be parsed as an IPv6 address"));
        }
        else if (!mcastConfig.groupV6.address().is_multicast()) {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("face_system.udp.mcast_group_v6: '" +
                                valueStr + "' is not a multicast address"));
        }
      }
      else if (key == "mcast_port_v6") {
        mcastConfig.groupV6.port(ConfigFile::parseNumber<uint16_t>(pair, "face_system.udp"));
      }
      else if (key == "mcast_ad_hoc") {
        bool wantAdHoc = ConfigFile::parseYesNo(pair, "face_system.udp");
        mcastConfig.linkType = wantAdHoc ? ndn::nfd::LINK_TYPE_AD_HOC : ndn::nfd::LINK_TYPE_MULTI_ACCESS;
      }
      else if (key == "whitelist") {
        mcastConfig.netifPredicate.parseWhitelist(value);
      }
      else if (key == "blacklist") {
        mcastConfig.netifPredicate.parseBlacklist(value);
      }
      else {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option face_system.udp." + key));
      }
    }

    if (!enableV4 && !enableV6 && !mcastConfig.isEnabled) {
      BOOST_THROW_EXCEPTION(ConfigFile::Error(
        "IPv4 and IPv6 UDP channels and UDP multicast have been disabled. "
        "Remove face_system.udp section to disable UDP channels or enable at least one of them."));
    }
  }

  if (context.isDryRun) {
    return;
  }

  if (enableV4) {
    udp::Endpoint endpoint(ip::udp::v4(), port);
    shared_ptr<UdpChannel> v4Channel = this->createChannel(endpoint, time::seconds(idleTimeout));
    if (wantListen && !v4Channel->isListening()) {
      v4Channel->listen(this->addFace, nullptr);
    }
    providedSchemes.insert("udp");
    providedSchemes.insert("udp4");
  }
  else if (providedSchemes.count("udp4") > 0) {
    NFD_LOG_WARN("Cannot close udp4 channel after its creation");
  }

  if (enableV6) {
    udp::Endpoint endpoint(ip::udp::v6(), port);
    shared_ptr<UdpChannel> v6Channel = this->createChannel(endpoint, time::seconds(idleTimeout));
    if (wantListen && !v6Channel->isListening()) {
      v6Channel->listen(this->addFace, nullptr);
    }
    providedSchemes.insert("udp");
    providedSchemes.insert("udp6");
  }
  else if (providedSchemes.count("udp6") > 0) {
    NFD_LOG_WARN("Cannot close udp6 channel after its creation");
  }

  if (m_mcastConfig.isEnabled != mcastConfig.isEnabled) {
    if (mcastConfig.isEnabled) {
      NFD_LOG_INFO("enabling multicast on " << mcastConfig.group);
      NFD_LOG_INFO("enabling multicast on " << mcastConfig.groupV6);
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
      NFD_LOG_INFO("changing IPv4 multicast group from " << m_mcastConfig.group <<
                   " to " << mcastConfig.group);
    }
    if (m_mcastConfig.groupV6 != mcastConfig.groupV6) {
      NFD_LOG_INFO("changing IPv6 multicast group from " << m_mcastConfig.groupV6 <<
                   " to " << mcastConfig.groupV6);
    }
    if (m_mcastConfig.netifPredicate != mcastConfig.netifPredicate) {
      NFD_LOG_INFO("changing whitelist/blacklist");
    }
  }

  // Even if there's no configuration change, we still need to re-apply configuration because
  // netifs may have changed.
  m_mcastConfig = mcastConfig;
  this->applyMcastConfig(context);
}

void
UdpFactory::createFace(const CreateFaceRequest& req,
                       const FaceCreatedCallback& onCreated,
                       const FaceCreationFailedCallback& onFailure)
{
  BOOST_ASSERT(req.remoteUri.isCanonical());

  if (req.localUri) {
    NFD_LOG_TRACE("Cannot create unicast UDP face with LocalUri");
    onFailure(406, "Unicast UDP faces cannot be created with a LocalUri");
    return;
  }

  if (req.params.persistency == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND) {
    NFD_LOG_TRACE("createFace does not support FACE_PERSISTENCY_ON_DEMAND");
    onFailure(406, "Outgoing UDP faces do not support on-demand persistency");
    return;
  }

  udp::Endpoint endpoint(ndn::ip::addressFromString(req.remoteUri.getHost()),
                         boost::lexical_cast<uint16_t>(req.remoteUri.getPort()));

  if (endpoint.address().is_multicast()) {
    NFD_LOG_TRACE("createFace does not support multicast faces");
    onFailure(406, "Cannot create multicast UDP faces");
    return;
  }

  if (req.params.wantLocalFields) {
    // UDP faces are never local
    NFD_LOG_TRACE("createFace cannot create non-local face with local fields enabled");
    onFailure(406, "Local fields can only be enabled on faces with local scope");
    return;
  }

  // very simple logic for now
  for (const auto& i : m_channels) {
    if ((i.first.address().is_v4() && endpoint.address().is_v4()) ||
        (i.first.address().is_v6() && endpoint.address().is_v6())) {
      i.second->connect(endpoint, req.params, onCreated, onFailure);
      return;
    }
  }

  NFD_LOG_TRACE("No channels available to connect to " << endpoint);
  onFailure(504, "No channels available to connect");
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const udp::Endpoint& localEndpoint,
                          time::nanoseconds idleTimeout)
{
  auto it = m_channels.find(localEndpoint);
  if (it != m_channels.end())
    return it->second;

  // check if the endpoint is already used by a multicast face
  if (m_mcastFaces.find(localEndpoint) != m_mcastFaces.end()) {
    BOOST_THROW_EXCEPTION(Error("Cannot create UDP channel on " +
                                boost::lexical_cast<std::string>(localEndpoint) +
                                ", endpoint already allocated for a UDP multicast face"));
  }

  auto channel = std::make_shared<UdpChannel>(localEndpoint, idleTimeout, m_wantCongestionMarking);
  m_channels[localEndpoint] = channel;

  return channel;
}

std::vector<shared_ptr<const Channel>>
UdpFactory::getChannels() const
{
  return getChannelsFromMap(m_channels);
}

shared_ptr<Face>
UdpFactory::createMulticastFace(const shared_ptr<const net::NetworkInterface>& netif,
                                const ip::address& localAddress,
                                const udp::Endpoint& multicastEndpoint)
{
  BOOST_ASSERT(multicastEndpoint.address().is_multicast());

  udp::Endpoint localEp(localAddress, multicastEndpoint.port());
  BOOST_ASSERT(localEp.protocol() == multicastEndpoint.protocol());

  auto mcastEp = multicastEndpoint;
  if (mcastEp.address().is_v6()) {
    // in IPv6, a scope id on the multicast address is always required
    auto mcastAddress = mcastEp.address().to_v6();
    mcastAddress.scope_id(netif->getIndex());
    mcastEp.address(mcastAddress);
  }

  // check if the local endpoint is already used by another multicast face
  auto it = m_mcastFaces.find(localEp);
  if (it != m_mcastFaces.end()) {
    if (it->second->getRemoteUri() == FaceUri(mcastEp))
      return it->second;
    else
      BOOST_THROW_EXCEPTION(Error("Cannot create UDP multicast face on " +
                                  boost::lexical_cast<std::string>(localEp) +
                                  ", endpoint already allocated for a different UDP multicast face"));
  }

  // check if the local endpoint is already used by a unicast channel
  if (m_channels.find(localEp) != m_channels.end()) {
    BOOST_THROW_EXCEPTION(Error("Cannot create UDP multicast face on " +
                                boost::lexical_cast<std::string>(localEp) +
                                ", endpoint already allocated for a UDP channel"));
  }

  ip::udp::socket rxSock(getGlobalIoService());
  MulticastUdpTransport::openRxSocket(rxSock, mcastEp, localAddress, netif);
  ip::udp::socket txSock(getGlobalIoService());
  MulticastUdpTransport::openTxSocket(txSock, udp::Endpoint(localAddress, 0), netif);

  GenericLinkService::Options options;
  options.allowCongestionMarking = m_wantCongestionMarking;
  auto linkService = make_unique<GenericLinkService>(options);
  auto transport = make_unique<MulticastUdpTransport>(mcastEp, std::move(rxSock), std::move(txSock),
                                                      m_mcastConfig.linkType);
  auto face = make_shared<Face>(std::move(linkService), std::move(transport));

  m_mcastFaces[localEp] = face;
  connectFaceClosedSignal(*face, [this, localEp] { m_mcastFaces.erase(localEp); });

  return face;
}

static optional<ip::address>
pickAddress(const net::NetworkInterface& netif, net::AddressFamily af)
{
  for (const auto& na : netif.getNetworkAddresses()) {
    if (na.getFamily() == af &&
        (na.getScope() == net::AddressScope::LINK || na.getScope() == net::AddressScope::GLOBAL)) {
      return na.getIp();
    }
  }
  return nullopt;
}

std::vector<shared_ptr<Face>>
UdpFactory::applyMcastConfigToNetif(const shared_ptr<const net::NetworkInterface>& netif)
{
  BOOST_ASSERT(netif != nullptr);

  if (!m_mcastConfig.isEnabled) {
    return {};
  }

  if (!netif->isUp()) {
    NFD_LOG_DEBUG("Not creating multicast faces on " << netif->getName() << ": netif is down");
    return {};
  }

  if (netif->isLoopback()) {
    NFD_LOG_DEBUG("Not creating multicast faces on " << netif->getName() << ": netif is loopback");
    return {};
  }

  if (!netif->canMulticast()) {
    NFD_LOG_DEBUG("Not creating multicast faces on " << netif->getName() << ": netif cannot multicast");
    return {};
  }

  if (!m_mcastConfig.netifPredicate(*netif)) {
    NFD_LOG_DEBUG("Not creating multicast faces on " << netif->getName() << ": rejected by whitelist/blacklist");
    return {};
  }

  std::vector<ip::address> addrs;
  for (auto af : {net::AddressFamily::V4, net::AddressFamily::V6}) {
    auto addr = pickAddress(*netif, af);
    if (addr)
      addrs.push_back(*addr);
  }

  if (addrs.empty()) {
    NFD_LOG_DEBUG("Not creating multicast faces on " << netif->getName() << ": no viable IP address");
    // keep an eye on new addresses
    m_netifConns[netif->getIndex()].addrAddConn =
      netif->onAddressAdded.connect(bind(&UdpFactory::applyMcastConfigToNetif, this, netif));
    return {};
  }

  NFD_LOG_DEBUG("Creating multicast faces on " << netif->getName());

  std::vector<shared_ptr<Face>> faces;
  for (const auto& addr : addrs) {
    auto face = this->createMulticastFace(netif, addr,
                                          addr.is_v4() ? m_mcastConfig.group : m_mcastConfig.groupV6);
    if (face->getId() == INVALID_FACEID) {
      // new face: register with forwarding
      this->addFace(face);
    }
    faces.push_back(std::move(face));
  }

  return faces;
}

void
UdpFactory::applyMcastConfig(const FaceSystem::ConfigContext& context)
{
  // collect old faces
  std::set<shared_ptr<Face>> facesToClose;
  boost::copy(m_mcastFaces | boost::adaptors::map_values,
              std::inserter(facesToClose, facesToClose.end()));

  // create faces if requested by config
  for (const auto& netif : netmon->listNetworkInterfaces()) {
    auto facesToKeep = this->applyMcastConfigToNetif(netif);
    for (const auto& face : facesToKeep) {
      // don't destroy face
      facesToClose.erase(face);
    }
  }

  // destroy old faces that are not needed in new configuration
  for (const auto& face : facesToClose) {
    face->close();
  }
}

} // namespace face
} // namespace nfd
