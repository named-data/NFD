/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "tcp-channel.hpp"

namespace ndn {

TcpChannel::TcpChannel(boost::asio::io_service& ioService,
                       const tcp::Endpoint& endpoint)
{
}

void
TcpChannel::listen(const tcp::Endpoint& endpoint)
{
}

void
TcpChannel::connect(const tcp::Endpoint& endpoint)
{
}

} // namespace ndn
