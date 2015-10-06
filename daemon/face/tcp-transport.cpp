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

#include "tcp-transport.hpp"

namespace nfd {
namespace face {

NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(StreamTransport, TcpTransport::protocol,
                                               "TcpTransport");

TcpTransport::TcpTransport(protocol::socket&& socket, ndn::nfd::FacePersistency persistency)
  : StreamTransport(std::move(socket))
{
  this->setLocalUri(FaceUri(m_socket.local_endpoint()));
  this->setRemoteUri(FaceUri(m_socket.remote_endpoint()));

  if (m_socket.local_endpoint().address().is_loopback() &&
      m_socket.remote_endpoint().address().is_loopback())
    this->setScope(ndn::nfd::FACE_SCOPE_LOCAL);
  else
    this->setScope(ndn::nfd::FACE_SCOPE_NON_LOCAL);

  this->setPersistency(persistency);
  this->setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  this->setMtu(MTU_UNLIMITED);

  NFD_LOG_FACE_INFO("Creating transport");
}

void
TcpTransport::beforeChangePersistency(ndn::nfd::FacePersistency newPersistency)
{
  if (newPersistency == ndn::nfd::FACE_PERSISTENCY_PERMANENT) {
    BOOST_THROW_EXCEPTION(
      std::invalid_argument("TcpTransport does not support FACE_PERSISTENCY_PERMANENT"));
  }
}

} // namespace face
} // namespace nfd
