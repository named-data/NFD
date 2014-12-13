/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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
#include "core/global-io.hpp"
#include "core/resolver.hpp"
#include "core/network-interface.hpp"

#if defined(__linux__)
#include <sys/socket.h>
#endif

namespace nfd {

using namespace boost::asio;

NFD_LOG_INIT("UdpFactory");

static const boost::asio::ip::address_v4 ALL_V4_ENDPOINT(
  boost::asio::ip::address_v4::from_string("0.0.0.0"));

static const boost::asio::ip::address_v6 ALL_V6_ENDPOINT(
  boost::asio::ip::address_v6::from_string("::"));

UdpFactory::UdpFactory(const std::string& defaultPort/* = "6363"*/)
  : m_defaultPort(defaultPort)
{
}

void
UdpFactory::prohibitEndpoint(const udp::Endpoint& endpoint)
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

  NFD_LOG_TRACE("prohibiting UDP " << endpoint);

  m_prohibitedEndpoints.insert(endpoint);
}

void
UdpFactory::prohibitAllIpv4Endpoints(const uint16_t port)
{
  using namespace boost::asio::ip;

  for (const NetworkInterfaceInfo& nic : listNetworkInterfaces()) {
    for (const address_v4& addr : nic.ipv4Addresses) {
      if (addr != ALL_V4_ENDPOINT) {
        prohibitEndpoint(udp::Endpoint(addr, port));
      }
    }

    if (nic.isBroadcastCapable() && nic.broadcastAddress != ALL_V4_ENDPOINT)
    {
      NFD_LOG_TRACE("prohibiting broadcast address: " << nic.broadcastAddress.to_string());
      prohibitEndpoint(udp::Endpoint(nic.broadcastAddress, port));
    }
  }

  prohibitEndpoint(udp::Endpoint(address::from_string("255.255.255.255"), port));
}

void
UdpFactory::prohibitAllIpv6Endpoints(const uint16_t port)
{
  using namespace boost::asio::ip;

  for (const NetworkInterfaceInfo& nic : listNetworkInterfaces()) {
    for (const address_v6& addr : nic.ipv6Addresses) {
      if (addr != ALL_V6_ENDPOINT) {
        prohibitEndpoint(udp::Endpoint(addr, port));
      }
    }
  }
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const udp::Endpoint& endpoint,
                          const time::seconds& timeout)
{
  NFD_LOG_DEBUG("Creating unicast channel " << endpoint);

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

  channel = make_shared<UdpChannel>(endpoint, timeout);
  m_channels[endpoint] = channel;
  prohibitEndpoint(endpoint);

  return channel;
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const std::string& localHost,
                          const std::string& localPort,
                          const time::seconds& timeout)
{
  return createChannel(UdpResolver::syncResolve(localHost, localPort), timeout);
}

shared_ptr<MulticastUdpFace>
UdpFactory::createMulticastFace(const udp::Endpoint& localEndpoint,
                                const udp::Endpoint& multicastEndpoint,
                                const std::string& networkInterfaceName /* "" */)
{
  //checking if the local and multicast endpoint are already in use for a multicast face
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

  shared_ptr<ip::udp::socket> receiveSocket =
    make_shared<ip::udp::socket>(ref(getGlobalIoService()));

  shared_ptr<ip::udp::socket> sendSocket =
    make_shared<ip::udp::socket>(ref(getGlobalIoService()));

  receiveSocket->open(multicastEndpoint.protocol());
  receiveSocket->set_option(ip::udp::socket::reuse_address(true));

  sendSocket->open(multicastEndpoint.protocol());
  sendSocket->set_option(ip::udp::socket::reuse_address(true));
  sendSocket->set_option(ip::multicast::enable_loopback(false));

  try {
    sendSocket->bind(udp::Endpoint(ip::address_v4::any(), multicastEndpoint.port()));
    receiveSocket->bind(multicastEndpoint);

    if (localEndpoint.address() != ip::address::from_string("0.0.0.0")) {
      sendSocket->set_option(ip::multicast::outbound_interface(localEndpoint.address().to_v4()));
    }
    sendSocket->set_option(ip::multicast::join_group(multicastEndpoint.address().to_v4(),
                                                     localEndpoint.address().to_v4()));

    receiveSocket->set_option(ip::multicast::join_group(multicastEndpoint.address().to_v4(),
                                                        localEndpoint.address().to_v4()));
  }
  catch (boost::system::system_error& e) {
    std::stringstream msg;
    msg << "Failed to properly configure the socket, check the address (" << e.what() << ")";
    throw Error(msg.str());
  }

#if defined(__linux__)
  //On linux system, if there are more than one MulticastUdpFace for the same multicast group but
  //bound on different network interfaces, the socket has to be bound with the specific interface
  //using SO_BINDTODEVICE, otherwise the face will receive packets also from other interfaces.
  //Without SO_BINDTODEVICE every MulticastUdpFace that have joined the same multicast group
  //on different interfaces will receive the same packet.
  //This applies only on linux, for OS X the ip::multicast::join_group is enough to get
  //the desired behaviour
  if (!networkInterfaceName.empty()) {
    if (::setsockopt(receiveSocket->native_handle(), SOL_SOCKET, SO_BINDTODEVICE,
                     networkInterfaceName.c_str(), networkInterfaceName.size()+1) == -1){
      throw Error("Cannot bind multicast face to " + networkInterfaceName
                  + " make sure you have CAP_NET_RAW capability" );
    }
  }

#endif

  multicastFace = make_shared<MulticastUdpFace>(receiveSocket, sendSocket,
                                                localEndpoint, multicastEndpoint);
  multicastFace->onFail += bind(&UdpFactory::afterFaceFailed, this, localEndpoint);

  m_multicastFaces[localEndpoint] = multicastFace;

  return multicastFace;
}

shared_ptr<MulticastUdpFace>
UdpFactory::createMulticastFace(const std::string& localIp,
                                const std::string& multicastIp,
                                const std::string& multicastPort,
                                const std::string& networkInterfaceName /* "" */)
{

  return createMulticastFace(UdpResolver::syncResolve(localIp,
                                                      multicastPort),
                             UdpResolver::syncResolve(multicastIp,
                                                      multicastPort),
                             networkInterfaceName);
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

  if (!uri.getPath().empty() && uri.getPath() != "/")
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
  onConnectFailed("No channels available to connect to " +
                  boost::lexical_cast<std::string>(endpoint));
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

std::list<shared_ptr<const Channel> >
UdpFactory::getChannels() const
{
  std::list<shared_ptr<const Channel> > channels;
  for (ChannelMap::const_iterator i = m_channels.begin(); i != m_channels.end(); ++i)
    {
      channels.push_back(i->second);
    }

  return channels;
}



} // namespace nfd
