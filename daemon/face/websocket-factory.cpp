/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

using namespace boost::asio;

WebSocketFactory::WebSocketFactory(const std::string& defaultPort)
  : m_defaultPort(defaultPort)
{
}

shared_ptr<WebSocketChannel>
WebSocketFactory::createChannel(const websocket::Endpoint& endpoint)
{
  shared_ptr<WebSocketChannel> channel = findChannel(endpoint);
  if (static_cast<bool>(channel))
    return channel;

  channel = make_shared<WebSocketChannel>(endpoint);
  m_channels[endpoint] = channel;

  return channel;
}

shared_ptr<WebSocketChannel>
WebSocketFactory::createChannel(const std::string& localIp, const std::string& port)
{
  using namespace boost::asio::ip;
  websocket::Endpoint endpoint(address::from_string(localIp), boost::lexical_cast<uint16_t>(port));
  return createChannel(endpoint);
}

shared_ptr<WebSocketChannel>
WebSocketFactory::findChannel(const websocket::Endpoint& localEndpoint)
{
  ChannelMap::iterator i = m_channels.find(localEndpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return shared_ptr<WebSocketChannel>();
}

void
WebSocketFactory::createFace(const FaceUri& uri,
                             ndn::nfd::FacePersistency persistency,
                             const FaceCreatedCallback& onCreated,
                             const FaceConnectFailedCallback& onConnectFailed)
{
  BOOST_THROW_EXCEPTION(Error("WebSocketFactory does not support 'createFace' operation"));
}

std::list<shared_ptr<const Channel> >
WebSocketFactory::getChannels() const
{
  std::list<shared_ptr<const Channel> > channels;
  for (ChannelMap::const_iterator i = m_channels.begin(); i != m_channels.end(); ++i)
    {
      channels.push_back(i->second);
    }

  return channels;
}

} // namespace nfd
