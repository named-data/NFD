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

#include "websocket-factory.hpp"

namespace nfd {
namespace face {

namespace ip = boost::asio::ip;

NFD_LOG_INIT(WebSocketFactory);
NFD_REGISTER_PROTOCOL_FACTORY(WebSocketFactory);

const std::string&
WebSocketFactory::getId() noexcept
{
  static std::string id("websocket");
  return id;
}

void
WebSocketFactory::doProcessConfig(OptionalConfigSection configSection,
                                  FaceSystem::ConfigContext& context)
{
  // websocket
  // {
  //   listen yes
  //   port 9696
  //   enable_v4 yes
  //   enable_v6 yes
  // }

  bool wantListen = false;
  uint16_t port = 9696;
  bool enableV4 = true;
  bool enableV6 = true;

  if (configSection) {
    wantListen = true;
    for (const auto& pair : *configSection) {
      const std::string& key = pair.first;

      if (key == "listen") {
        wantListen = ConfigFile::parseYesNo(pair, "face_system.websocket");
      }
      else if (key == "port") {
        port = ConfigFile::parseNumber<uint16_t>(pair, "face_system.websocket");
      }
      else if (key == "enable_v4") {
        enableV4 = ConfigFile::parseYesNo(pair, "face_system.websocket");
      }
      else if (key == "enable_v6") {
        enableV6 = ConfigFile::parseYesNo(pair, "face_system.websocket");
      }
      else {
        NDN_THROW(ConfigFile::Error("Unrecognized option face_system.websocket." + key));
      }
    }
  }

  if (!enableV4 && !enableV6) {
    NDN_THROW(ConfigFile::Error(
      "IPv4 and IPv6 WebSocket channels have been disabled. Remove face_system.websocket section "
      "to disable WebSocket channels or enable at least one channel type."));
  }

  if (context.isDryRun) {
    return;
  }

  if (!wantListen) {
    if (!m_channels.empty()) {
      NFD_LOG_WARN("Cannot disable WebSocket channels after initialization");
    }
    return;
  }

  if (enableV4) {
    websocket::Endpoint endpoint(ip::tcp::v4(), port);
    auto v4Channel = this->createChannel(endpoint);
    if (!v4Channel->isListening()) {
      v4Channel->listen(this->addFace);
    }
  }

  if (enableV6) {
    websocket::Endpoint endpoint(ip::tcp::v6(), port);
    auto v6Channel = this->createChannel(endpoint);
    if (!v6Channel->isListening()) {
      v6Channel->listen(this->addFace);
    }
  }
}

shared_ptr<WebSocketChannel>
WebSocketFactory::createChannel(const websocket::Endpoint& endpoint)
{
  auto it = m_channels.find(endpoint);
  if (it != m_channels.end())
    return it->second;

  auto channel = make_shared<WebSocketChannel>(endpoint);
  m_channels[endpoint] = channel;
  return channel;
}

std::vector<shared_ptr<const Channel>>
WebSocketFactory::doGetChannels() const
{
  return getChannelsFromMap(m_channels);
}

} // namespace face
} // namespace nfd
