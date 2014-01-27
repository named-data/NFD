/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_TCP_CHANNEL_HPP
#define NFD_FACE_TCP_CHANNEL_HPP

// #include "session-based-channel.hpp"

#include "common.hpp"
// #include <boost/asio/ip/tcp.hpp>

namespace tcp {
typedef boost::asio::ip::tcp::endpoint Endpoint;
} // namespace tcp

namespace ndn {

class TcpChannel // : protected SessionBasedChannel
{
public:
  TcpChannel(boost::asio::io_service& ioService, const tcp::Endpoint& endpoint);

  void
  listen(const tcp::Endpoint& endpoint);

  void
  connect(const tcp::Endpoint& endpoint);

private:
  tcp::Endpoint m_localEndpoint;
};

} // namespace ndn
 
#endif // NFD_FACE_TCP_CHANNEL_HPP
