/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "tcp-factory.hpp"
#include "core/resolver.hpp"
#include "core/logger.hpp"
#include "core/network-interface.hpp"

NFD_LOG_INIT("TcpFactory");

namespace nfd {

TcpFactory::TcpFactory(const std::string& defaultPort/* = "6363"*/)
  : m_defaultPort(defaultPort)
{

}

void
TcpFactory::prohibitEndpoint(const tcp::Endpoint& endpoint)
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

  NFD_LOG_TRACE("prohibiting TCP " <<
                endpoint.address().to_string() << ":" << endpoint.port());

  m_prohibitedEndpoints.insert(endpoint);
}

void
TcpFactory::prohibitAllIpv4Endpoints(const uint16_t port)
{
  using namespace boost::asio::ip;

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
          prohibitEndpoint(tcp::Endpoint(*j, port));
        }
    }
}

void
TcpFactory::prohibitAllIpv6Endpoints(const uint16_t port)
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
          prohibitEndpoint(tcp::Endpoint(*j, port));
        }
    }
}

shared_ptr<TcpChannel>
TcpFactory::createChannel(const tcp::Endpoint& endpoint)
{
  shared_ptr<TcpChannel> channel = findChannel(endpoint);
  if(static_cast<bool>(channel))
    return channel;

  channel = make_shared<TcpChannel>(boost::cref(endpoint));
  m_channels[endpoint] = channel;
  prohibitEndpoint(endpoint);

  NFD_LOG_DEBUG("Channel [" << endpoint << "] created");
  return channel;
}

shared_ptr<TcpChannel>
TcpFactory::createChannel(const std::string& localHost, const std::string& localPort)
{
  return createChannel(TcpResolver::syncResolve(localHost, localPort));
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
                       const FaceCreatedCallback& onCreated,
                       const FaceConnectFailedCallback& onConnectFailed)
{
  resolver::AddressSelector addressSelector = resolver::AnyAddress();
  if (uri.getScheme() == "tcp4")
    addressSelector = resolver::Ipv4Address();
  else if (uri.getScheme() == "tcp6")
    addressSelector = resolver::Ipv6Address();

  if (!uri.getPath().empty())
    {
      onConnectFailed("Invalid URI");
    }

  TcpResolver::asyncResolve(uri.getHost(),
                            uri.getPort().empty() ? m_defaultPort : uri.getPort(),
                            bind(&TcpFactory::continueCreateFaceAfterResolve, this, _1,
                                 onCreated, onConnectFailed),
                            onConnectFailed,
                            addressSelector);
}

void
TcpFactory::continueCreateFaceAfterResolve(const tcp::Endpoint& endpoint,
                                           const FaceCreatedCallback& onCreated,
                                           const FaceConnectFailedCallback& onConnectFailed)
{
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

} // namespace nfd
