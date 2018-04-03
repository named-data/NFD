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

#include "tcp-factory.hpp"

#include <ndn-cxx/net/address-converter.hpp>

namespace nfd {
namespace face {

namespace ip = boost::asio::ip;

NFD_LOG_INIT("TcpFactory");
NFD_REGISTER_PROTOCOL_FACTORY(TcpFactory);

const std::string&
TcpFactory::getId()
{
  static std::string id("tcp");
  return id;
}

TcpFactory::TcpFactory(const CtorParams& params)
  : ProtocolFactory(params)
{
}

void
TcpFactory::processConfig(OptionalConfigSection configSection,
                          FaceSystem::ConfigContext& context)
{
  // tcp
  // {
  //   listen yes
  //   port 6363
  //   enable_v4 yes
  //   enable_v6 yes
  // }

  m_wantCongestionMarking = context.generalConfig.wantCongestionMarking;

  if (!configSection) {
    if (!context.isDryRun && !m_channels.empty()) {
      NFD_LOG_WARN("Cannot disable tcp4 and tcp6 channels after initialization");
    }
    return;
  }

  bool wantListen = true;
  uint16_t port = 6363;
  bool enableV4 = true;
  bool enableV6 = true;
  IpAddressPredicate local;
  bool isLocalConfigured = false;

  for (const auto& pair : *configSection) {
    const std::string& key = pair.first;

    if (key == "listen") {
      wantListen = ConfigFile::parseYesNo(pair, "face_system.tcp");
    }
    else if (key == "port") {
      port = ConfigFile::parseNumber<uint16_t>(pair, "face_system.tcp");
    }
    else if (key == "enable_v4") {
      enableV4 = ConfigFile::parseYesNo(pair, "face_system.tcp");
    }
    else if (key == "enable_v6") {
      enableV6 = ConfigFile::parseYesNo(pair, "face_system.tcp");
    }
    else if (key == "local") {
      isLocalConfigured = true;
      for (const auto& localPair : pair.second) {
        const std::string& localKey = localPair.first;
        if (localKey == "whitelist") {
          local.parseWhitelist(localPair.second);
        }
        else if (localKey == "blacklist") {
          local.parseBlacklist(localPair.second);
        }
        else {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option face_system.tcp.local." + localKey));
        }
      }
    }
    else {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option face_system.tcp." + key));
    }
  }
  if (!isLocalConfigured) {
    local.assign({{"subnet", "127.0.0.0/8"}, {"subnet", "::1/128"}}, {});
  }

  if (!enableV4 && !enableV6) {
    BOOST_THROW_EXCEPTION(ConfigFile::Error(
      "IPv4 and IPv6 TCP channels have been disabled. Remove face_system.tcp section to disable "
      "TCP channels or enable at least one channel type."));
  }

  if (!context.isDryRun) {
    providedSchemes.insert("tcp");

    if (enableV4) {
      tcp::Endpoint endpoint(ip::tcp::v4(), port);
      shared_ptr<TcpChannel> v4Channel = this->createChannel(endpoint);
      if (wantListen && !v4Channel->isListening()) {
        v4Channel->listen(this->addFace, nullptr);
      }
      providedSchemes.insert("tcp4");
    }
    else if (providedSchemes.count("tcp4") > 0) {
      NFD_LOG_WARN("Cannot close tcp4 channel after its creation");
    }

    if (enableV6) {
      tcp::Endpoint endpoint(ip::tcp::v6(), port);
      shared_ptr<TcpChannel> v6Channel = this->createChannel(endpoint);
      if (wantListen && !v6Channel->isListening()) {
        v6Channel->listen(this->addFace, nullptr);
      }
      providedSchemes.insert("tcp6");
    }
    else if (providedSchemes.count("tcp6") > 0) {
      NFD_LOG_WARN("Cannot close tcp6 channel after its creation");
    }

    m_local = std::move(local);
  }
}

void
TcpFactory::createFace(const CreateFaceRequest& req,
                       const FaceCreatedCallback& onCreated,
                       const FaceCreationFailedCallback& onFailure)
{
  BOOST_ASSERT(req.remoteUri.isCanonical());

  if (req.localUri) {
    NFD_LOG_TRACE("Cannot create unicast TCP face with LocalUri");
    onFailure(406, "Unicast TCP faces cannot be created with a LocalUri");
    return;
  }

  if (req.params.persistency == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND) {
    NFD_LOG_TRACE("createFace does not support FACE_PERSISTENCY_ON_DEMAND");
    onFailure(406, "Outgoing TCP faces do not support on-demand persistency");
    return;
  }

  tcp::Endpoint endpoint(ndn::ip::addressFromString(req.remoteUri.getHost()),
                         boost::lexical_cast<uint16_t>(req.remoteUri.getPort()));

  // a canonical tcp4/tcp6 FaceUri cannot have a multicast address
  BOOST_ASSERT(!endpoint.address().is_multicast());

  if (req.params.wantLocalFields && !endpoint.address().is_loopback()) {
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

shared_ptr<TcpChannel>
TcpFactory::createChannel(const tcp::Endpoint& endpoint)
{
  auto it = m_channels.find(endpoint);
  if (it != m_channels.end())
    return it->second;

  auto channel = make_shared<TcpChannel>(endpoint, m_wantCongestionMarking,
                                         bind(&TcpFactory::determineFaceScopeFromAddresses, this, _1, _2));
  m_channels[endpoint] = channel;
  return channel;
}

std::vector<shared_ptr<const Channel>>
TcpFactory::getChannels() const
{
  return getChannelsFromMap(m_channels);
}

ndn::nfd::FaceScope
TcpFactory::determineFaceScopeFromAddresses(const boost::asio::ip::address& localAddress,
                                            const boost::asio::ip::address& remoteAddress) const
{
  if (m_local(localAddress) && m_local(remoteAddress)) {
    return ndn::nfd::FACE_SCOPE_LOCAL;
  }
  return ndn::nfd::FACE_SCOPE_NON_LOCAL;
}

} // namespace face
} // namespace nfd
