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

#include "websocket-factory.hpp"

namespace nfd {

namespace ip = boost::asio::ip;

shared_ptr<WebSocketChannel>
WebSocketFactory::createChannel(const websocket::Endpoint& endpoint)
{
  auto channel = findChannel(endpoint);
  if (channel)
    return channel;

  channel = make_shared<WebSocketChannel>(endpoint);
  m_channels[endpoint] = channel;

  return channel;
}

shared_ptr<WebSocketChannel>
WebSocketFactory::createChannel(const std::string& localIp, const std::string& localPort)
{
  websocket::Endpoint endpoint(ip::address::from_string(localIp),
                               boost::lexical_cast<uint16_t>(localPort));
  return createChannel(endpoint);
}

void
WebSocketFactory::createFace(const FaceUri& uri,
                             ndn::nfd::FacePersistency persistency,
                             bool wantLocalFieldsEnabled,
                             const FaceCreatedCallback& onCreated,
                             const FaceCreationFailedCallback& onFailure)
{
  onFailure(406, "Unsupported protocol");
}

std::vector<shared_ptr<const Channel>>
WebSocketFactory::getChannels() const
{
  std::vector<shared_ptr<const Channel>> channels;
  channels.reserve(m_channels.size());

  for (const auto& i : m_channels)
    channels.push_back(i.second);

  return channels;
}

shared_ptr<WebSocketChannel>
WebSocketFactory::findChannel(const websocket::Endpoint& endpoint) const
{
  auto i = m_channels.find(endpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return nullptr;
}

} // namespace nfd
