/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "udp-factory.hpp"
#include "core/global-io.hpp"
#include "core/resolver.hpp"

namespace nfd {

using namespace boost::asio;
  
NFD_LOG_INIT("UdpFactory");

UdpFactory::UdpFactory(const std::string& defaultPort/* = "6363"*/)
  : m_defaultPort(defaultPort)
{
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const udp::Endpoint& endpoint,
                          const time::Duration& timeout)
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

  return channel;
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const std::string& localHost,
                          const std::string& localPort,
                          const time::Duration& timeout)
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

  multicastFace = make_shared<MulticastUdpFace>(boost::cref(clientSocket));
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
  
  UdpResolver::asyncResolve(uri.getDomain(),
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
  
  // very simple logic for now
  
  for (ChannelMap::iterator channel = m_channels.begin();
       channel != m_channels.end();
       ++channel)
  {
    if ((channel->first.address().is_v4() && endpoint.address().is_v4()) ||
        (channel->first.address().is_v6() && endpoint.address().is_v6()))
    {
      channel->second->connect(endpoint, onCreated);
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
