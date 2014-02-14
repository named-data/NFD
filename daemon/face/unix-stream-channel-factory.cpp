/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "unix-stream-channel-factory.hpp"

#if BOOST_VERSION >= 104800
#include <boost/filesystem.hpp> // for canonical()
#endif

namespace nfd {

UnixStreamChannelFactory::UnixStreamChannelFactory(boost::asio::io_service& ioService)
  : m_ioService(ioService)
{
}

shared_ptr<UnixStreamChannel>
UnixStreamChannelFactory::create(const std::string& unixSocketPath)
{
#if BOOST_VERSION >= 104800
  boost::filesystem::path p(unixSocketPath);
  p = boost::filesystem::canonical(p.parent_path()) / p.filename();
  unix_stream::Endpoint endpoint(p.string());
#else
  unix_stream::Endpoint endpoint(unixSocketPath);
#endif

  shared_ptr<UnixStreamChannel> channel = find(endpoint);
  if (channel)
    return channel;

  channel = make_shared<UnixStreamChannel>(boost::ref(m_ioService),
                                           boost::cref(endpoint));
  m_channels[endpoint] = channel;
  return channel;
}

shared_ptr<UnixStreamChannel>
UnixStreamChannelFactory::find(const unix_stream::Endpoint& endpoint)
{
  ChannelMap::iterator i = m_channels.find(endpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return shared_ptr<UnixStreamChannel>();
}

} // namespace nfd
