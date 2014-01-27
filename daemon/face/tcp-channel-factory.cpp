/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "tcp-channel-factory.hpp"

namespace ndn {

shared_ptr<TcpChannel>
TcpChannelFactory::create(const tcp::Endpoint& endpoint)
{
  return shared_ptr<ndn::TcpChannel>();
}

shared_ptr<TcpChannel>
TcpChannelFactory::create(const std::string& localHost, const std::string& localPort)
{
  return shared_ptr<ndn::TcpChannel>();
}

shared_ptr<TcpChannel>
TcpChannelFactory::find(const tcp::Endpoint& localEndpoint)
{
  return shared_ptr<ndn::TcpChannel>();
}

} // namespace ndn
