/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_INTERNAL_TRANSPORT_HPP
#define NFD_DAEMON_FACE_INTERNAL_TRANSPORT_HPP

#include "transport.hpp"

#include <ndn-cxx/transport/transport.hpp>

namespace nfd {
namespace face {

/** \brief Abstracts a transport that can be paired with another.
 */
class InternalTransportBase
{
public:
  virtual
  ~InternalTransportBase() = default;

  virtual void
  receivePacket(const Block& packet) = 0;
};

/** \brief Implements a forwarder-side transport that can be paired with another transport.
 */
class InternalForwarderTransport final : public Transport, public InternalTransportBase
{
public:
  explicit
  InternalForwarderTransport(const FaceUri& localUri = FaceUri("internal://"),
                             const FaceUri& remoteUri = FaceUri("internal://"),
                             ndn::nfd::FaceScope scope = ndn::nfd::FACE_SCOPE_LOCAL,
                             ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_POINT_TO_POINT);

  void
  setPeer(InternalTransportBase* peer)
  {
    m_peer = peer;
  }

  void
  receivePacket(const Block& packet) final;

protected:
  void
  doClose() final;

private:
  void
  doSend(const Block& packet, const EndpointId& endpoint) final;

private:
  NFD_LOG_MEMBER_DECL();

  InternalTransportBase* m_peer = nullptr;
};

/** \brief Implements a client-side transport that can be paired with an InternalForwarderTransport.
 */
class InternalClientTransport final : public ndn::Transport, public InternalTransportBase
{
public:
  ~InternalClientTransport() final;

  /** \brief Connect to a forwarder-side transport.
   *  \param forwarder the forwarder-side transport to connect to; may be nullptr
   *
   *  The connected forwarder-side transport will be disconnected automatically if this method
   *  is called again, or if that transport is closed.
   *  It's safe to use InternalClientTransport without a connected forwarder-side transport:
   *  all sent packets would be lost, and nothing would be received.
   */
  void
  connectToForwarder(InternalForwarderTransport* forwarder);

  void
  receivePacket(const Block& packet) final;

  void
  send(const Block& wire) final;

  void
  send(const Block& header, const Block& payload) final;

  void
  close() final
  {
  }

  void
  pause() final
  {
  }

  void
  resume() final
  {
  }

private:
  NFD_LOG_MEMBER_DECL();

  InternalForwarderTransport* m_forwarder = nullptr;
  signal::ScopedConnection m_fwTransportStateConn;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_INTERNAL_TRANSPORT_HPP
