/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "udp-factory.hpp"
#include "core/global-io.hpp"
#include "core/resolver.hpp"
#include "core/network-interface.hpp"

namespace nfd {

using namespace boost::asio;

NFD_LOG_INIT("UdpFactory");

UdpFactory::UdpFactory(const std::string& defaultPort/* = "6363"*/)
  : m_defaultPort(defaultPort)
{
}



void
UdpFactory::prohibitEndpoint(const udp::Endpoint& endpoint)
{
  using namespace boost::asio::ip;

  static const address_v4 ALL_V4_ENDPOINT(address_v4::from_string("0.0.0.0"));
  static const address_v6 ALL_V6_ENDPOINT(address_v6::from_string("::"));

  const address& address = endpoint.address();

  if (address.is_v4() && address == ALL_V4_ENDPOINT)
    {
      prohibitAllIpv4Endpoints(endpoint.port());
    }
  else if (endpoint.address().is_v6() && address == ALL_V6_ENDPOINT)
    {
      prohibitAllIpv6Endpoints(endpoint.port());
    }

  NFD_LOG_TRACE("prohibiting UDP " <<
                endpoint.address().to_string() << ":" << endpoint.port());

  m_prohibitedEndpoints.insert(endpoint);
}

void
UdpFactory::prohibitAllIpv4Endpoints(const uint16_t port)
{
  using namespace boost::asio::ip;

  static const address_v4 INVALID_BROADCAST(address_v4::from_string("0.0.0.0"));

  const std::list<shared_ptr<NetworkInterfaceInfo> > nicList(listNetworkInterfaces());

  for (std::list<shared_ptr<NetworkInterfaceInfo> >::const_iterator i = nicList.begin();
       i != nicList.end();
       ++i)
    {
      const shared_ptr<NetworkInterfaceInfo>& nic = *i;
      const std::vector<address_v4>& ipv4Addresses = nic->ipv4Addresses;

      for (std::vector<address_v4>::const_iterator j = ipv4Addresses.begin();
           j != ipv4Addresses.end();
           ++j)
        {
          prohibitEndpoint(udp::Endpoint(*j, port));
        }

      if (nic->isBroadcastCapable() && nic->broadcastAddress != INVALID_BROADCAST)
        {
          NFD_LOG_TRACE("prohibiting broadcast address: " << nic->broadcastAddress.to_string());
          prohibitEndpoint(udp::Endpoint(nic->broadcastAddress, port));
        }
    }

  prohibitEndpoint(udp::Endpoint(address::from_string("255.255.255.255"), port));
}

void
UdpFactory::prohibitAllIpv6Endpoints(const uint16_t port)
{
  using namespace boost::asio::ip;

  const std::list<shared_ptr<NetworkInterfaceInfo> > nicList(listNetworkInterfaces());

  for (std::list<shared_ptr<NetworkInterfaceInfo> >::const_iterator i = nicList.begin();
       i != nicList.end();
       ++i)
    {
      const shared_ptr<NetworkInterfaceInfo>& nic = *i;
      const std::vector<address_v6>& ipv6Addresses = nic->ipv6Addresses;

      for (std::vector<address_v6>::const_iterator j = ipv6Addresses.begin();
           j != ipv6Addresses.end();
           ++j)
        {
          prohibitEndpoint(udp::Endpoint(*j, port));
        }
    }
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const udp::Endpoint& endpoint,
                          const time::seconds& timeout)
{
  NFD_LOG_DEBUG("Creating unicast " << endpoint);

  shared_ptr<UdpChannel> channel = findChannel(endpoint);
  if (static_cast<bool>(channel))
    return channel;


  //checking if the endpoint is already in use for multicast face
  shared_ptr<MulticastUdpFace> multicast = findMulticastFace(endpoint);
  if (static_cast<bool>(multicast))
    throw Error("Cannot create the requested UDP unicast channel, local "
                "endpoint is already allocated for a UDP multicast face");

  if (endpoint.address().is_multicast()) {
    throw Error("This method is only for unicast channel. The provided "
                "endpoint is multicast. Use createMulticastFace to "
                "create a multicast face");
  }

  channel = make_shared<UdpChannel>(boost::cref(endpoint),
                                    timeout);
  m_channels[endpoint] = channel;
  prohibitEndpoint(endpoint);

  return channel;
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const std::string& localHost,
                          const std::string& localPort,
                          const time::seconds& timeout)
{
  return createChannel(UdpResolver::syncResolve(localHost, localPort),
                       timeout);
}

shared_ptr<MulticastUdpFace>
UdpFactory::createMulticastFace(const udp::Endpoint& localEndpoint,
                                const udp::Endpoint& multicastEndpoint)
{
  //checking if the local and musticast endpoint are already in use for a multicast face
  shared_ptr<MulticastUdpFace> multicastFace = findMulticastFace(localEndpoint);
  if (static_cast<bool>(multicastFace)) {
    if (multicastFace->getMulticastGroup() == multicastEndpoint)
      return multicastFace;
    else
      throw Error("Cannot create the requested UDP multicast face, local "
                  "endpoint is already allocated for a UDP multicast face "
                  "on a different multicast group");
  }

  //checking if the local endpoint is already in use for an unicast channel
  shared_ptr<UdpChannel> unicast = findChannel(localEndpoint);
  if (static_cast<bool>(unicast)) {
    throw Error("Cannot create the requested UDP multicast face, local "
                "endpoint is already allocated for a UDP unicast channel");
  }

  if (m_prohibitedEndpoints.find(multicastEndpoint) != m_prohibitedEndpoints.end()) {
    throw Error("Cannot create the requested UDP multicast face, "
                "remote endpoint is owned by this NFD instance");
  }

  if (localEndpoint.address().is_v6() || multicastEndpoint.address().is_v6()) {
    throw Error("IPv6 multicast is not supported yet. Please provide an IPv4 address");
  }

  if (localEndpoint.port() != multicastEndpoint.port()) {
    throw Error("Cannot create the requested UDP multicast face, "
                "both endpoints should have the same port number. ");
  }

  if (!multicastEndpoint.address().is_multicast()) {
    throw Error("Cannot create the requested UDP multicast face, "
                "the multicast group given as input is not a multicast address");
  }

  shared_ptr<ip::udp::socket> clientSocket =
    make_shared<ip::udp::socket>(boost::ref(getGlobalIoService()));

  clientSocket->open(multicastEndpoint.protocol());

  clientSocket->set_option(ip::udp::socket::reuse_address(true));

  try {
    clientSocket->bind(multicastEndpoint);

    if (localEndpoint.address() != ip::address::from_string("0.0.0.0")) {
      clientSocket->set_option(ip::multicast::outbound_interface(localEndpoint.address().to_v4()));
    }
    clientSocket->set_option(ip::multicast::join_group(multicastEndpoint.address().to_v4(),
                                                       localEndpoint.address().to_v4()));
  }
  catch (boost::system::system_error& e) {
    std::stringstream msg;
    msg << "Failed to properly configure the socket, check the address (" << e.what() << ")";
    throw Error(msg.str());
  }

  clientSocket->set_option(ip::multicast::enable_loopback(false));

  multicastFace = make_shared<MulticastUdpFace>(boost::cref(clientSocket), localEndpoint);
  multicastFace->onFail += bind(&UdpFactory::afterFaceFailed, this, localEndpoint);

  m_multicastFaces[localEndpoint] = multicastFace;

  return multicastFace;
}

shared_ptr<MulticastUdpFace>
UdpFactory::createMulticastFace(const std::string& localIp,
                                const std::string& multicastIp,
                                const std::string& multicastPort)
{

  return createMulticastFace(UdpResolver::syncResolve(localIp,
                                                      multicastPort),
                             UdpResolver::syncResolve(multicastIp,
                                                      multicastPort));
}

void
UdpFactory::createFace(const FaceUri& uri,
                       const FaceCreatedCallback& onCreated,
                       const FaceConnectFailedCallback& onConnectFailed)
{
  resolver::AddressSelector addressSelector = resolver::AnyAddress();
  if (uri.getScheme() == "udp4")
    addressSelector = resolver::Ipv4Address();
  else if (uri.getScheme() == "udp6")
    addressSelector = resolver::Ipv6Address();

  if (!uri.getPath().empty())
    {
      onConnectFailed("Invalid URI");
    }

  UdpResolver::asyncResolve(uri.getHost(),
                            uri.getPort().empty() ? m_defaultPort : uri.getPort(),
                            bind(&UdpFactory::continueCreateFaceAfterResolve, this, _1,
                                 onCreated, onConnectFailed),
                            onConnectFailed,
                            addressSelector);

}

void
UdpFactory::continueCreateFaceAfterResolve(const udp::Endpoint& endpoint,
                                           const FaceCreatedCallback& onCreated,
                                           const FaceConnectFailedCallback& onConnectFailed)
{
  if (endpoint.address().is_multicast()) {
    onConnectFailed("The provided address is multicast. Please use createMulticastFace method");
    return;
  }

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

shared_ptr<UdpChannel>
UdpFactory::findChannel(const udp::Endpoint& localEndpoint)
{
  ChannelMap::iterator i = m_channels.find(localEndpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return shared_ptr<UdpChannel>();
}

shared_ptr<MulticastUdpFace>
UdpFactory::findMulticastFace(const udp::Endpoint& localEndpoint)
{
  MulticastFaceMap::iterator i = m_multicastFaces.find(localEndpoint);
  if (i != m_multicastFaces.end())
    return i->second;
  else
    return shared_ptr<MulticastUdpFace>();
}

void
UdpFactory::afterFaceFailed(udp::Endpoint& endpoint)
{
  NFD_LOG_DEBUG("afterFaceFailed: " << endpoint);
  m_multicastFaces.erase(endpoint);
}


} // namespace nfd
