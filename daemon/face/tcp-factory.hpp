/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_TCP_FACTORY_HPP
#define NFD_FACE_TCP_FACTORY_HPP

#include "protocol-factory.hpp"
#include "tcp-channel.hpp"

namespace nfd {

class TcpFactory : public ProtocolFactory
{
public:
  /**
   * \brief Exception of TcpFactory
   */
  struct Error : public ProtocolFactory::Error
  {
    Error(const std::string& what) : ProtocolFactory::Error(what) {}
  };

  explicit
  TcpFactory(const std::string& defaultPort = "6363");

  /**
   * \brief Create TCP-based channel using tcp::Endpoint
   *
   * tcp::Endpoint is really an alias for boost::asio::ip::tcp::endpoint.
   *
   * If this method called twice with the same endpoint, only one channel
   * will be created.  The second call will just retrieve the existing
   * channel.
   *
   * \returns always a valid pointer to a TcpChannel object, an exception
   *          is thrown if it cannot be created.
   *
   * \throws TcpFactory::Error
   *
   * \see http://www.boost.org/doc/libs/1_42_0/doc/html/boost_asio/reference/ip__tcp/endpoint.html
   *      for details on ways to create tcp::Endpoint
   */
  shared_ptr<TcpChannel>
  createChannel(const tcp::Endpoint& localEndpoint);

  /**
   * \brief Create TCP-based channel using specified host and port number
   *
   * This method will attempt to resolve the provided host and port numbers
   * and will throw TcpFactory::Error when channel cannot be created.
   *
   * Note that this call will **BLOCK** until resolution is done or failed.
   *
   * \throws TcpFactory::Error
   */
  shared_ptr<TcpChannel>
  createChannel(const std::string& localHost, const std::string& localPort);

  // from Factory

  virtual void
  createFace(const FaceUri& uri,
             const FaceCreatedCallback& onCreated,
             const FaceConnectFailedCallback& onConnectFailed);

private:
  /**
   * \brief Look up TcpChannel using specified local endpoint
   *
   * \returns shared pointer to the existing TcpChannel object
   *          or empty shared pointer when such channel does not exist
   *
   * \throws never
   */
  shared_ptr<TcpChannel>
  findChannel(const tcp::Endpoint& localEndpoint);

  void
  continueCreateFaceAfterResolve(const tcp::Endpoint& endpoint,
                                 const FaceCreatedCallback& onCreated,
                                 const FaceConnectFailedCallback& onConnectFailed);

private:
  typedef std::map< tcp::Endpoint, shared_ptr<TcpChannel> > ChannelMap;
  ChannelMap m_channels;

  std::string m_defaultPort;
};

} // namespace nfd

#endif // NFD_FACE_TCP_FACTORY_HPP
