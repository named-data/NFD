/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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
#include "face.hpp"
#include "generic-link-service.hpp"
#include "unix-stream-transport.hpp"
#include "common/global.hpp"

#include <boost/filesystem.hpp>

namespace nfd::face {

NFD_LOG_INIT(UnixStreamChannel);

UnixStreamChannel::UnixStreamChannel(const unix_stream::Endpoint& endpoint,
                                     bool wantCongestionMarking)
  : m_endpoint(endpoint)
  , m_wantCongestionMarking(wantCongestionMarking)
  , m_acceptor(getGlobalIoService())
{
  setUri(FaceUri(m_endpoint));
  NFD_LOG_CHAN_INFO("Creating channel");
}

UnixStreamChannel::~UnixStreamChannel()
{
  if (isListening()) {
    // use the non-throwing variants during destruction and ignore any errors
    boost::system::error_code ec;
    m_acceptor.close(ec);
    NFD_LOG_CHAN_TRACE("Removing socket file");
    boost::filesystem::remove(m_endpoint.path(), ec);
  }
}

void
UnixStreamChannel::listen(const FaceCreatedCallback& onFaceCreated,
                          const FaceCreationFailedCallback& onAcceptFailed,
                          int backlog)
{
  if (isListening()) {
    NFD_LOG_CHAN_WARN("Already listening");
    return;
  }

  namespace fs = boost::filesystem;

  fs::path socketPath = m_endpoint.path();
  // ensure parent directory exists
  fs::path parent = socketPath.parent_path();
  if (!parent.empty() && fs::create_directories(parent)) {
    NFD_LOG_CHAN_TRACE("Created directory " << parent);
  }

  boost::system::error_code ec;
  fs::file_type type = fs::symlink_status(socketPath).type();
  if (type == fs::socket_file) {
    // if the socket file already exists, there may be another instance
    // of NFD running on the system: make sure we don't steal its socket
    boost::asio::local::stream_protocol::socket socket(getGlobalIoService());
    socket.connect(m_endpoint, ec);
    NFD_LOG_CHAN_TRACE("connect() on existing socket file returned: " << ec.message());
    if (!ec) {
      // someone answered, leave the socket alone
      ec = boost::system::errc::make_error_code(boost::system::errc::address_in_use);
      NDN_THROW_NO_STACK(fs::filesystem_error("UnixStreamChannel::listen", socketPath, ec));
    }
    else if (ec == boost::asio::error::connection_refused ||
             ec == boost::asio::error::timed_out) {
      // no one is listening on the remote side, we can safely remove the stale socket
      NFD_LOG_CHAN_DEBUG("Removing stale socket file");
      fs::remove(socketPath);
    }
  }
  else if (type != fs::file_not_found) {
    // the file exists but is not a socket: this is a fatal error as we cannot
    // safely overwrite the file without potentially risking data loss
    ec = boost::system::errc::make_error_code(boost::system::errc::not_a_socket);
    NDN_THROW_NO_STACK(fs::filesystem_error("UnixStreamChannel::listen", socketPath, ec));
  }

  try {
    m_acceptor.open();
    m_acceptor.bind(m_endpoint);
    m_acceptor.listen(backlog);
  }
  catch (const boost::system::system_error& e) {
    // exceptions thrown by Boost.Asio are very terse, add more context
    NDN_THROW_NO_STACK(fs::filesystem_error("UnixStreamChannel::listen: "s + e.std::runtime_error::what(),
                                            socketPath, e.code()));
  }

  // do this here so that, even if the calls below fail,
  // the destructor will still remove the socket file
  m_isListening = true;

  fs::permissions(socketPath, fs::owner_read | fs::group_read | fs::others_read |
                              fs::owner_write | fs::group_write | fs::others_write);

  accept(onFaceCreated, onAcceptFailed);
  NFD_LOG_CHAN_DEBUG("Started listening");
}

void
UnixStreamChannel::accept(const FaceCreatedCallback& onFaceCreated,
                          const FaceCreationFailedCallback& onAcceptFailed)
{
  m_acceptor.async_accept([=] (const boost::system::error_code& error,
                               boost::asio::local::stream_protocol::socket socket) {
    if (error) {
      if (error != boost::asio::error::operation_aborted) {
        NFD_LOG_CHAN_DEBUG("Accept failed: " << error.message());
        if (onAcceptFailed)
          onAcceptFailed(500, "Accept failed: " + error.message());
      }
      return;
    }

    NFD_LOG_CHAN_TRACE("Incoming connection via fd " << socket.native_handle());

    GenericLinkService::Options options;
    options.allowCongestionMarking = m_wantCongestionMarking;
    auto linkService = make_unique<GenericLinkService>(options);
    auto transport = make_unique<UnixStreamTransport>(std::move(socket));
    auto face = make_shared<Face>(std::move(linkService), std::move(transport));
    face->setChannel(weak_from_this());

    ++m_size;
    connectFaceClosedSignal(*face, [this] { --m_size; });

    onFaceCreated(face);

    // prepare accepting the next connection
    accept(onFaceCreated, onAcceptFailed);
  });
}

} // namespace nfd::face
