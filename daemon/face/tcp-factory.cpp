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

#include "tcp-factory.hpp"
#include "core/logger.hpp"
#include "core/network-interface.hpp"
#include "core/global-io.hpp"

NFD_LOG_INIT("TcpFactory");

namespace nfd {

static const boost::asio::ip::address_v4 ALL_V4_ENDPOINT(
  boost::asio::ip::address_v4::from_string("0.0.0.0"));

static const boost::asio::ip::address_v6 ALL_V6_ENDPOINT(
  boost::asio::ip::address_v6::from_string("::"));

TcpFactory::TcpFactory(const std::string& defaultPort/* = "6363"*/)
  : m_defaultPort(defaultPort)
{
}

void
TcpFactory::prohibitEndpoint(const tcp::Endpoint& endpoint)
{
  using namespace boost::asio::ip;

  const address& address = endpoint.address();

  if (address.is_v4() && address == ALL_V4_ENDPOINT)
    {
      prohibitAllIpv4Endpoints(endpoint.port());
    }
  else if (endpoint.address().is_v6() && address == ALL_V6_ENDPOINT)
    {
      prohibitAllIpv6Endpoints(endpoint.port());
    }

  NFD_LOG_TRACE("prohibiting TCP " << endpoint);

  m_prohibitedEndpoints.insert(endpoint);
}

void
TcpFactory::prohibitAllIpv4Endpoints(const uint16_t port)
{
  using namespace boost::asio::ip;

  for (const NetworkInterfaceInfo& nic : listNetworkInterfaces()) {
    for (const address_v4& addr : nic.ipv4Addresses) {
      if (addr != ALL_V4_ENDPOINT) {
        prohibitEndpoint(tcp::Endpoint(addr, port));
      }
    }
  }
}

void
TcpFactory::prohibitAllIpv6Endpoints(const uint16_t port)
{
  using namespace boost::asio::ip;

  for (const NetworkInterfaceInfo& nic : listNetworkInterfaces()) {
    for (const address_v6& addr : nic.ipv6Addresses) {
      if (addr != ALL_V6_ENDPOINT) {
        prohibitEndpoint(tcp::Endpoint(addr, port));
      }
    }
  }
}

shared_ptr<TcpChannel>
TcpFactory::createChannel(const tcp::Endpoint& endpoint)
{
  shared_ptr<TcpChannel> channel = findChannel(endpoint);
  if (static_cast<bool>(channel))
    return channel;

  channel = make_shared<TcpChannel>(endpoint);
  m_channels[endpoint] = channel;
  prohibitEndpoint(endpoint);

  NFD_LOG_DEBUG("Channel [" << endpoint << "] created");
  return channel;
}

shared_ptr<TcpChannel>
TcpFactory::createChannel(const std::string& localIp, const std::string& localPort)
{
  using namespace boost::asio::ip;
  tcp::Endpoint endpoint(address::from_string(localIp), boost::lexical_cast<uint16_t>(localPort));
  return createChannel(endpoint);
}

shared_ptr<TcpChannel>
TcpFactory::findChannel(const tcp::Endpoint& localEndpoint)
{
  ChannelMap::iterator i = m_channels.find(localEndpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return shared_ptr<TcpChannel>();
}

void
TcpFactory::createFace(const FaceUri& uri,
                       ndn::nfd::FacePersistency persistency,
                       const FaceCreatedCallback& onCreated,
                       const FaceConnectFailedCallback& onConnectFailed)
{
  if (persistency != ndn::nfd::FACE_PERSISTENCY_PERSISTENT) {
    BOOST_THROW_EXCEPTION(Error("TcpFactory only supports persistent face"));
  }

  BOOST_ASSERT(uri.isCanonical());
  boost::asio::ip::address ipAddress = boost::asio::ip::address::from_string(uri.getHost());
  tcp::Endpoint endpoint(ipAddress, boost::lexical_cast<uint16_t>(uri.getPort()));

  if (m_prohibitedEndpoints.find(endpoint) != m_prohibitedEndpoints.end())
    {
      onConnectFailed("Requested endpoint is prohibited "
                      "(reserved by this NFD or disallowed by face management protocol)");
      return;
    }

  // very simple logic for now
  for (ChannelMap::iterator channel = m_channels.begin();
       channel != m_channels.end();
       ++channel)
    {
      if ((channel->first.address().is_v4() && endpoint.address().is_v4()) ||
          (channel->first.address().is_v6() && endpoint.address().is_v6()))
        {
          channel->second->connect(endpoint, onCreated, onConnectFailed);
          return;
        }
    }
  onConnectFailed("No channels available to connect to "
                  + boost::lexical_cast<std::string>(endpoint));
}

std::list<shared_ptr<const Channel> >
TcpFactory::getChannels() const
{
  std::list<shared_ptr<const Channel> > channels;
  for (ChannelMap::const_iterator i = m_channels.begin(); i != m_channels.end(); ++i)
    {
      channels.push_back(i->second);
    }

  return channels;
}

} // namespace nfd
