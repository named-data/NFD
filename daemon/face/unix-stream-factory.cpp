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

#include "unix-stream-factory.hpp"

#include <boost/filesystem.hpp>

namespace nfd {
namespace face {

NFD_LOG_INIT(UnixStreamFactory);
NFD_REGISTER_PROTOCOL_FACTORY(UnixStreamFactory);

const std::string&
UnixStreamFactory::getId() noexcept
{
  static std::string id("unix");
  return id;
}

void
UnixStreamFactory::doProcessConfig(OptionalConfigSection configSection,
                                   FaceSystem::ConfigContext& context)
{
  // unix
  // {
  //   path /var/run/nfd.sock
  // }

  m_wantCongestionMarking = context.generalConfig.wantCongestionMarking;

  if (!configSection) {
    if (!context.isDryRun && !m_channels.empty()) {
      NFD_LOG_WARN("Cannot disable Unix channel after initialization");
    }
    return;
  }

  std::string path = "/var/run/nfd.sock";

  for (const auto& pair : *configSection) {
    const std::string& key = pair.first;
    const ConfigSection& value = pair.second;

    if (key == "path") {
      path = value.get_value<std::string>();
    }
    else {
      NDN_THROW(ConfigFile::Error("Unrecognized option face_system.unix." + key));
    }
  }

  if (context.isDryRun) {
    return;
  }

  auto channel = this->createChannel(path);
  if (!channel->isListening()) {
    channel->listen(this->addFace, nullptr);
  }
}

shared_ptr<UnixStreamChannel>
UnixStreamFactory::createChannel(const std::string& unixSocketPath)
{
  boost::filesystem::path p(unixSocketPath);
  p = boost::filesystem::canonical(p.parent_path()) / p.filename();
  unix_stream::Endpoint endpoint(p.string());

  auto it = m_channels.find(endpoint);
  if (it != m_channels.end())
    return it->second;

  auto channel = make_shared<UnixStreamChannel>(endpoint, m_wantCongestionMarking);
  m_channels[endpoint] = channel;
  return channel;
}

std::vector<shared_ptr<const Channel>>
UnixStreamFactory::doGetChannels() const
{
  return getChannelsFromMap(m_channels);
}

} // namespace face
} // namespace nfd
