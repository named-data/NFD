/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "tcp-factory.hpp"
#include "core/global-io.hpp"
#include "core/resolver.hpp"

namespace nfd {

TcpFactory::TcpFactory(const std::string& defaultPort/* = "6363"*/)
  : m_defaultPort(defaultPort)
{
}

shared_ptr<TcpChannel>
TcpFactory::createChannel(const tcp::Endpoint& endpoint)
{
  shared_ptr<TcpChannel> channel = findChannel(endpoint);
  if(static_cast<bool>(channel))
    return channel;

  channel = make_shared<TcpChannel>(boost::ref(getGlobalIoService()), boost::cref(endpoint));
  m_channels[endpoint] = channel;
  return channel;
}

shared_ptr<TcpChannel>
TcpFactory::createChannel(const std::string& localHost, const std::string& localPort)
{
  using boost::asio::ip::tcp;

  tcp::resolver::query query(localHost, localPort);
  tcp::resolver resolver(getGlobalIoService());

  tcp::resolver::iterator end;
  tcp::resolver::iterator i = resolver.resolve(query);
  if (i == end)
    return shared_ptr<TcpChannel>();

  return createChannel(*i);
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

  using boost::asio::ip::tcp;
  Resolver<tcp>::asyncResolve(uri.getDomain(),
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
