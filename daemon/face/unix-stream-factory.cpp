/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "unix-stream-factory.hpp"

#include <boost/filesystem.hpp> // for canonical()

namespace nfd {

shared_ptr<UnixStreamChannel>
UnixStreamFactory::createChannel(const std::string& unixSocketPath)
{
  boost::filesystem::path p(unixSocketPath);
  p = boost::filesystem::canonical(p.parent_path()) / p.filename();
  unix_stream::Endpoint endpoint(p.string());

  shared_ptr<UnixStreamChannel> channel = findChannel(endpoint);
  if (channel)
    return channel;

  channel = make_shared<UnixStreamChannel>(boost::cref(endpoint));
  m_channels[endpoint] = channel;
  return channel;
}

shared_ptr<UnixStreamChannel>
UnixStreamFactory::findChannel(const unix_stream::Endpoint& endpoint)
{
  ChannelMap::iterator i = m_channels.find(endpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return shared_ptr<UnixStreamChannel>();
}

void
UnixStreamFactory::createFace(const FaceUri& uri,
                              const FaceCreatedCallback& onCreated,
                              const FaceConnectFailedCallback& onConnectFailed)
{
  throw Error("UnixStreamFactory does not support 'createFace' operation");
}

} // namespace nfd
