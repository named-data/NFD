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

#include "tcp-factory.hpp"
#include "core/logger.hpp"
#include "core/network-interface.hpp"

namespace nfd {

namespace ip = boost::asio::ip;

NFD_LOG_INIT("TcpFactory");

void
TcpFactory::prohibitEndpoint(const tcp::Endpoint& endpoint)
{
  if (endpoint.address().is_v4() &&
      endpoint.address() == ip::address_v4::any()) {
    prohibitAllIpv4Endpoints(endpoint.port());
  }
  else if (endpoint.address().is_v6() &&
           endpoint.address() == ip::address_v6::any()) {
    prohibitAllIpv6Endpoints(endpoint.port());
  }

  NFD_LOG_TRACE("prohibiting TCP " << endpoint);
  m_prohibitedEndpoints.insert(endpoint);
}

void
TcpFactory::prohibitAllIpv4Endpoints(uint16_t port)
{
  for (const NetworkInterfaceInfo& nic : listNetworkInterfaces()) {
    for (const auto& addr : nic.ipv4Addresses) {
      if (addr != ip::address_v4::any()) {
        prohibitEndpoint(tcp::Endpoint(addr, port));
      }
    }
  }
}

void
TcpFactory::prohibitAllIpv6Endpoints(uint16_t port)
{
  for (const NetworkInterfaceInfo& nic : listNetworkInterfaces()) {
    for (const auto& addr : nic.ipv6Addresses) {
      if (addr != ip::address_v6::any()) {
        prohibitEndpoint(tcp::Endpoint(addr, port));
      }
    }
  }
}

shared_ptr<TcpChannel>
TcpFactory::createChannel(const tcp::Endpoint& endpoint)
{
  auto channel = findChannel(endpoint);
  if (channel)
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
  tcp::Endpoint endpoint(ip::address::from_string(localIp),
                         boost::lexical_cast<uint16_t>(localPort));
  return createChannel(endpoint);
}

void
TcpFactory::createFace(const FaceUri& uri,
                       ndn::nfd::FacePersistency persistency,
                       bool wantLocalFieldsEnabled,
                       const FaceCreatedCallback& onCreated,
                       const FaceCreationFailedCallback& onFailure)
{
  BOOST_ASSERT(uri.isCanonical());

  if (persistency != ndn::nfd::FACE_PERSISTENCY_PERSISTENT) {
    NFD_LOG_TRACE("createFace only supports FACE_PERSISTENCY_PERSISTENT");
    onFailure(406, "Outgoing TCP faces only support persistent persistency");
    return;
  }

  tcp::Endpoint endpoint(ip::address::from_string(uri.getHost()),
                         boost::lexical_cast<uint16_t>(uri.getPort()));

  if (endpoint.address().is_multicast()) {
   NFD_LOG_TRACE("createFace cannot create multicast faces");
   onFailure(406, "Cannot create multicast TCP faces");
   return;
  }

  if (m_prohibitedEndpoints.find(endpoint) != m_prohibitedEndpoints.end()) {
    NFD_LOG_TRACE("Requested endpoint is prohibited "
                  "(reserved by NFD or disallowed by face management protocol)");
    onFailure(406, "Requested endpoint is prohibited");
    return;
  }

  if (wantLocalFieldsEnabled && !endpoint.address().is_loopback()) {
    NFD_LOG_TRACE("createFace cannot create non-local face with local fields enabled");
    onFailure(406, "Local fields can only be enabled on faces with local scope");
    return;
  }

  // very simple logic for now
  for (const auto& i : m_channels) {
    if ((i.first.address().is_v4() && endpoint.address().is_v4()) ||
        (i.first.address().is_v6() && endpoint.address().is_v6())) {
      i.second->connect(endpoint, wantLocalFieldsEnabled, onCreated, onFailure);
      return;
    }
  }

  NFD_LOG_TRACE("No channels available to connect to " + boost::lexical_cast<std::string>(endpoint));
  onFailure(504, "No channels available to connect");
}

std::vector<shared_ptr<const Channel>>
TcpFactory::getChannels() const
{
  std::vector<shared_ptr<const Channel>> channels;
  channels.reserve(m_channels.size());

  for (const auto& i : m_channels)
    channels.push_back(i.second);

  return channels;
}

shared_ptr<TcpChannel>
TcpFactory::findChannel(const tcp::Endpoint& localEndpoint) const
{
  auto i = m_channels.find(localEndpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return nullptr;
}

} // namespace nfd
