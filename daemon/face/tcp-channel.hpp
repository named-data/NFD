/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_TCP_CHANNEL_HPP
#define NFD_FACE_TCP_CHANNEL_HPP

#include "common.hpp"
#include "core/monotonic_deadline_timer.hpp"

namespace tcp {
typedef boost::asio::ip::tcp::endpoint Endpoint;
} // namespace tcp

namespace ndn {

/**
 * \brief Class implementing TCP-based channel to create faces
 *
 * Channel can create faces as a response to incoming TCP
 * connections (TcpChannel::listen needs to be called for that
 * to work) or explicitly after using TcpChannel::connect method.
 */
class TcpChannel // : protected SessionBasedChannel
{
public:
  /**
   * \brief Prototype for the callback called when face is created
   *        (as a response to incoming connection or after connection
   *        is established)
   */
  typedef function<void(const shared_ptr<TcpFace>& newFace)> FaceCreatedCallback;

  /**
   * \brief Prototype for the callback that is called when face is failed to
   *        get created
   */
  typedef function<void(const std::string& reason)> ConnectFailedCallback;
  
  /**
   * \brief Create TCP channel for the local endpoint
   *
   * To enable creation faces upon incoming connections,
   * one needs to explicitly call TcpChannel::listen method.
   */
  TcpChannel(boost::asio::io_service& ioService,
             const tcp::Endpoint& localEndpoint);

  /**
   * \brief Enable listening on the local endpoint, accept connections,
   *        and create faces when remote host makes a connection
   * \param backlog The maximum length of the queue of pending incoming
   *        connections
   */
  void
  listen(int backlog = boost::asio::ip::tcp::acceptor::max_connections);

  /**
   * \brief Create a face by establishing connection to remote endpoint
   */
  void
  connect(const tcp::Endpoint& remoteEndpoint,
          const time::Duration& timeout = time::seconds(4));

  /**
   * \brief Create a face by establishing connection to the specified
   *        remote host and remote port
   *
   * This method will never blocks and will return immediately. All
   * necessary hostname and port resolution and connection will happen
   * in asynchronous mode.
   *
   * If connection cannot be established within specified timeout, it
   * will be aborted.
   */
  void
  connect(const std::string& remoteHost, const std::string& remotePort,
          const time::Duration& timeout = time::seconds(4));
  
private:
  void
  handleConnection(const boost::system::error_code& error,
                   const shared_ptr<boost::asio::ip::tcp::socket>& socket);

  void
  handleSuccessfulConnect(const boost::system::error_code& error,
                          const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                          const shared_ptr<boost::asio::monotonic_deadline_timer>& timer);
  
  void
  handleFailedConnect(const boost::system::error_code& error,
                      const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                      const shared_ptr<boost::asio::monotonic_deadline_timer>& timer);

  void
  handleEndpointResoution(const boost::system::error_code& error,
                          boost::asio::ip::tcp::resolver::iterator remoteEndpoint,
                          const shared_ptr<boost::asio::ip::tcp::socket>& socket,
                          const shared_ptr<boost::asio::monotonic_deadline_timer>& timer);
  
private:
  boost::asio::io_service& m_ioService;
  tcp::Endpoint m_localEndpoint;

  bool isListening;
  shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
};

} // namespace ndn
 
#endif // NFD_FACE_TCP_CHANNEL_HPP
