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

#include "unix-stream-transport.hpp"

namespace nfd::face {

namespace local = boost::asio::local;

NFD_LOG_MEMBER_INIT_SPECIALIZED(StreamTransport<local::stream_protocol>, UnixStreamTransport);

UnixStreamTransport::UnixStreamTransport(local::stream_protocol::socket&& socket)
  : StreamTransport(std::move(socket))
{
  static_assert(
    std::is_same_v<std::remove_cv_t<local::stream_protocol::socket::native_handle_type>, int>,
    "The native handle type for UnixStreamTransport sockets must be 'int'"
  );

  this->setLocalUri(FaceUri(m_socket.local_endpoint()));
  this->setRemoteUri(FaceUri::fromFd(m_socket.native_handle()));
  this->setScope(ndn::nfd::FACE_SCOPE_LOCAL);
  this->setPersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  this->setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  this->setMtu(MTU_UNLIMITED);

  NFD_LOG_FACE_DEBUG("Creating transport");
}

} // namespace nfd::face
