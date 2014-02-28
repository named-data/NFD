/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_UNIX_STREAM_FACTORY_HPP
#define NFD_FACE_UNIX_STREAM_FACTORY_HPP

#include "protocol-factory.hpp"
#include "unix-stream-channel.hpp"

namespace nfd {

class UnixStreamFactory : public ProtocolFactory
{
public:
  /**
   * \brief Exception of UnixStreamFactory
   */
  struct Error : public ProtocolFactory::Error
  {
    Error(const std::string& what) : ProtocolFactory::Error(what) {}
  };

  /**
   * \brief Create stream-oriented Unix channel using specified socket path
   *
   * If this method is called twice with the same path, only one channel
   * will be created.  The second call will just retrieve the existing
   * channel.
   *
   * \returns always a valid pointer to a UnixStreamChannel object,
   *          an exception will be thrown if the channel cannot be created.
   *
   * \throws UnixStreamFactory::Error
   */
  shared_ptr<UnixStreamChannel>
  create(const std::string& unixSocketPath);

private:
  /**
   * \brief Look up UnixStreamChannel using specified endpoint
   *
   * \returns shared pointer to the existing UnixStreamChannel object
   *          or empty shared pointer when such channel does not exist
   *
   * \throws never
   */
  shared_ptr<UnixStreamChannel>
  find(const unix_stream::Endpoint& endpoint);

private:
  typedef std::map< unix_stream::Endpoint, shared_ptr<UnixStreamChannel> > ChannelMap;
  ChannelMap m_channels;
};

} // namespace nfd

#endif // NFD_FACE_UNIX_STREAM_FACTORY_HPP
