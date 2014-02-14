/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_UNIX_STREAM_CHANNEL_FACTORY_HPP
#define NFD_FACE_UNIX_STREAM_CHANNEL_FACTORY_HPP

#include "channel-factory.hpp"
#include "unix-stream-channel.hpp"

namespace nfd {

class UnixStreamChannelFactory : public ChannelFactory<unix_stream::Endpoint, UnixStreamChannel>
{
public:
  /**
   * \brief Exception of UnixStreamChannelFactory
   */
  struct Error : public ChannelFactory<unix_stream::Endpoint, UnixStreamChannel>::Error
  {
    Error(const std::string& what)
      : ChannelFactory<unix_stream::Endpoint, UnixStreamChannel>::Error(what)
    {
    }
  };

  explicit
  UnixStreamChannelFactory(boost::asio::io_service& ioService);

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
   * \throws UnixStreamChannelFactory::Error
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
  boost::asio::io_service& m_ioService;
};

} // namespace nfd

#endif // NFD_FACE_UNIX_STREAM_CHANNEL_FACTORY_HPP
