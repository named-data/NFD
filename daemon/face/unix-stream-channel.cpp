/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "unix-stream-channel.hpp"
#include "unix-stream-face.hpp"
#include "core/global-io.hpp"

#include <boost/filesystem.hpp>
#include <sys/stat.h> // for chmod()

namespace nfd {

NFD_LOG_INIT("UnixStreamChannel");

using namespace boost::asio::local;

UnixStreamChannel::UnixStreamChannel(const unix_stream::Endpoint& endpoint)
  : m_acceptor(getGlobalIoService())
  , m_endpoint(endpoint)
{
  setUri(FaceUri(m_endpoint));
}

UnixStreamChannel::~UnixStreamChannel()
{
  if (isListening()) {
    // use the non-throwing variants during destruction
    // and ignore any errors
    boost::system::error_code error;
    m_acceptor.close(error);
    NFD_LOG_DEBUG("[" << m_endpoint << "] Removing socket file");
    boost::filesystem::remove(m_endpoint.path(), error);
  }
}

void
UnixStreamChannel::listen(const FaceCreatedCallback& onFaceCreated,
                          const ConnectFailedCallback& onAcceptFailed,
                          int backlog/* = acceptor::max_connections*/)
{
  if (isListening()) {
    NFD_LOG_WARN("[" << m_endpoint << "] Already listening");
    return;
  }

  namespace fs = boost::filesystem;

  fs::path socketPath(m_endpoint.path());
  fs::file_type type = fs::symlink_status(socketPath).type();

  if (type == fs::socket_file) {
    boost::system::error_code error;
    stream_protocol::socket socket(getGlobalIoService());
    socket.connect(m_endpoint, error);
    NFD_LOG_TRACE("[" << m_endpoint << "] connect() on existing socket file returned: "
                  + error.message());
    if (!error) {
      // someone answered, leave the socket alone
      throw Error("Socket file at " + m_endpoint.path()
                  + " belongs to another NFD process");
    }
    else if (error == boost::asio::error::connection_refused ||
             error == boost::asio::error::timed_out) {
      // no one is listening on the remote side,
      // we can safely remove the stale socket
      NFD_LOG_DEBUG("[" << m_endpoint << "] Removing stale socket file");
      fs::remove(socketPath);
    }
  }
  else if (type != fs::file_not_found) {
    throw Error(m_endpoint.path() + " already exists and is not a socket file");
  }

  m_acceptor.open();
  m_acceptor.bind(m_endpoint);
  m_acceptor.listen(backlog);

  if (::chmod(m_endpoint.path().c_str(), 0666) < 0) {
    throw Error("chmod(" + m_endpoint.path() + ") failed: " + std::strerror(errno));
  }

  // start accepting connections
  accept(onFaceCreated, onAcceptFailed);
}

void
UnixStreamChannel::accept(const FaceCreatedCallback &onFaceCreated,
                          const ConnectFailedCallback &onAcceptFailed)
{
  auto socket = make_shared<stream_protocol::socket>(ref(getGlobalIoService()));

  m_acceptor.async_accept(*socket,
                          bind(&UnixStreamChannel::handleAccept, this,
                               boost::asio::placeholders::error,
                               socket, onFaceCreated, onAcceptFailed));
}

void
UnixStreamChannel::handleAccept(const boost::system::error_code& error,
                                const shared_ptr<stream_protocol::socket>& socket,
                                const FaceCreatedCallback& onFaceCreated,
                                const ConnectFailedCallback& onAcceptFailed)
{
  if (error) {
    if (error == boost::asio::error::operation_aborted) // when the socket is closed by someone
      return;

    NFD_LOG_DEBUG("[" << m_endpoint << "] Accept failed: " << error.message());
    if (onAcceptFailed)
      onAcceptFailed(error.message());
    return;
  }

  NFD_LOG_DEBUG("[" << m_endpoint << "] Incoming connection");

  // prepare accepting the next connection
  accept(onFaceCreated, onAcceptFailed);

  shared_ptr<UnixStreamFace> face = make_shared<UnixStreamFace>(socket);
  onFaceCreated(face);
}

} // namespace nfd
