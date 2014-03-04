/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "unix-stream-channel.hpp"
#include "core/global-io.hpp"

#include <boost/filesystem.hpp>
#include <sys/stat.h> // for chmod()

namespace nfd {

NFD_LOG_INIT("UnixStreamChannel");

using namespace boost::asio::local;

UnixStreamChannel::UnixStreamChannel(const unix_stream::Endpoint& endpoint)
  : m_endpoint(endpoint)
{
  this->setUri(FaceUri(endpoint));
}

UnixStreamChannel::~UnixStreamChannel()
{
  if (m_acceptor)
    {
      // use the non-throwing variants during destruction
      // and ignore any errors
      boost::system::error_code error;
      m_acceptor->close(error);
      boost::filesystem::remove(m_endpoint.path(), error);
    }
}

void
UnixStreamChannel::listen(const FaceCreatedCallback& onFaceCreated,
                          const ConnectFailedCallback& onAcceptFailed,
                          int backlog/* = acceptor::max_connections*/)
{
  if (m_acceptor) // already listening
    return;

  namespace fs = boost::filesystem;
  fs::path p(m_endpoint.path());
  if (fs::symlink_status(p).type() == fs::socket_file)
    {
      // for safety, remove an already existing file only if it's a socket
      fs::remove(p);
    }

  m_acceptor = make_shared<stream_protocol::acceptor>(boost::ref(getGlobalIoService()));
  m_acceptor->open(m_endpoint.protocol());
  m_acceptor->bind(m_endpoint);
  m_acceptor->listen(backlog);

  if (::chmod(m_endpoint.path().c_str(), 0666) < 0)
    {
      throw Error("Failed to chmod() socket file at " + m_endpoint.path());
    }

  shared_ptr<stream_protocol::socket> clientSocket =
    make_shared<stream_protocol::socket>(boost::ref(getGlobalIoService()));
  m_acceptor->async_accept(*clientSocket,
                           bind(&UnixStreamChannel::handleSuccessfulAccept, this, _1,
                                clientSocket, onFaceCreated, onAcceptFailed));
}

void
UnixStreamChannel::createFace(const shared_ptr<stream_protocol::socket>& socket,
                              const FaceCreatedCallback& onFaceCreated)
{
  shared_ptr<UnixStreamFace> face = make_shared<UnixStreamFace>(boost::cref(socket));
  onFaceCreated(face);
}

void
UnixStreamChannel::handleSuccessfulAccept(const boost::system::error_code& error,
                                          const shared_ptr<stream_protocol::socket>& socket,
                                          const FaceCreatedCallback& onFaceCreated,
                                          const ConnectFailedCallback& onAcceptFailed)
{
  if (error) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    NFD_LOG_DEBUG("Connection failed: "
                  << error.category().message(error.value()));

    onAcceptFailed("Connection failed: " +
                   error.category().message(error.value()));
    return;
  }

  // prepare accepting the next connection
  shared_ptr<stream_protocol::socket> clientSocket =
    make_shared<stream_protocol::socket>(boost::ref(getGlobalIoService()));
  m_acceptor->async_accept(*clientSocket,
                           bind(&UnixStreamChannel::handleSuccessfulAccept, this, _1,
                                clientSocket, onFaceCreated, onAcceptFailed));

  NFD_LOG_DEBUG("[" << m_endpoint << "] << Incoming connection");
  createFace(socket, onFaceCreated);
}

} // namespace nfd
