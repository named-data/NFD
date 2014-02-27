/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "unix-stream-channel-factory.hpp"
#include "core/global-io.hpp"

#include <boost/filesystem.hpp> // for canonical()

namespace nfd {

UnixStreamChannelFactory::UnixStreamChannelFactory()
{
}

shared_ptr<UnixStreamChannel>
UnixStreamChannelFactory::create(const std::string& unixSocketPath)
{
  boost::filesystem::path p(unixSocketPath);
  p = boost::filesystem::canonical(p.parent_path()) / p.filename();
  unix_stream::Endpoint endpoint(p.string());

  shared_ptr<UnixStreamChannel> channel = find(endpoint);
  if (channel)
    return channel;

  channel = make_shared<UnixStreamChannel>(boost::ref(getGlobalIoService()),
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
