/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_UNIX_STREAM_TRANSPORT_HPP
#define NFD_DAEMON_FACE_UNIX_STREAM_TRANSPORT_HPP

#include "stream-transport.hpp"

#include <boost/asio/local/stream_protocol.hpp>

#ifndef NFD_HAVE_UNIX_SOCKETS
#error "Cannot include this file when UNIX sockets are not available"
#endif

namespace nfd::face {

NFD_LOG_MEMBER_DECL_SPECIALIZED(StreamTransport<boost::asio::local::stream_protocol>);

/**
 * \brief A Transport that communicates on a stream-oriented Unix domain socket.
 */
class UnixStreamTransport final : public StreamTransport<boost::asio::local::stream_protocol>
{
public:
  explicit
  UnixStreamTransport(boost::asio::local::stream_protocol::socket&& socket);
};

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_UNIX_STREAM_TRANSPORT_HPP
