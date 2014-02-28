/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "tcp-factory.hpp"
#include "core/global-io.hpp"

namespace nfd {

shared_ptr<TcpChannel>
TcpFactory::create(const tcp::Endpoint& endpoint)
{
  shared_ptr<TcpChannel> channel = find(endpoint);
  if(static_cast<bool>(channel))
    return channel;

  channel = make_shared<TcpChannel>(boost::ref(getGlobalIoService()), boost::cref(endpoint));
  m_channels[endpoint] = channel;
  return channel;
}

shared_ptr<TcpChannel>
TcpFactory::create(const std::string& localHost, const std::string& localPort)
{
  using boost::asio::ip::tcp;

  tcp::resolver::query query(localHost, localPort);
  tcp::resolver resolver(getGlobalIoService());

  tcp::resolver::iterator end;
  tcp::resolver::iterator i = resolver.resolve(query);
  if (i == end)
    return shared_ptr<TcpChannel>();

  return create(*i);
}

shared_ptr<TcpChannel>
TcpFactory::find(const tcp::Endpoint& localEndpoint)
{
  ChannelMap::iterator i = m_channels.find(localEndpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return shared_ptr<TcpChannel>();
}

} // namespace nfd
