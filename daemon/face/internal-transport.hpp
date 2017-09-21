/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

/** \brief abstracts a transport that can be paired with another
 */
class InternalTransportBase
{
public:
  virtual
  ~InternalTransportBase() = default;

  /** \brief causes the transport to receive a link-layer packet
   */
  virtual void
  receiveFromLink(const Block& packet) = 0;

  signal::Signal<InternalTransportBase, Block> afterSend;

protected:
  DECLARE_SIGNAL_EMIT(afterSend)
};

/** \brief implements a forwarder-side transport that can be paired with another
 */
class InternalForwarderTransport : public Transport, public InternalTransportBase
{
public:
  InternalForwarderTransport(const FaceUri& localUri = FaceUri("internal://"),
                             const FaceUri& remoteUri = FaceUri("internal://"),
                             ndn::nfd::FaceScope scope = ndn::nfd::FACE_SCOPE_LOCAL,
                             ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_POINT_TO_POINT);

  void
  receiveFromLink(const Block& packet) override;

protected:
  void
  doClose() override;

private:
  void
  doSend(Packet&& packet) override;

private:
  NFD_LOG_INCLASS_DECLARE();
};

/** \brief implements a client-side transport that can be paired with another
 */
class InternalClientTransport : public ndn::Transport, public InternalTransportBase
{
public:
  /** \brief connect to a forwarder-side transport
   *  \param forwarderTransport the forwarder-side transport to connect to; may be nullptr
   *
   *  The connected forwarder-side transport will be disconnected automatically if this method
   *  is called again, or if that transport is closed.
   *  It's safe to use InternalClientTransport without a connected forwarder-side transport:
   *  all sent packets would be lost, and nothing would be received.
   */
  void
  connectToForwarder(InternalForwarderTransport* forwarderTransport);

  void
  receiveFromLink(const Block& packet) override;

  void
  close() override
  {
  }

  void
  pause() override
  {
  }

  void
  resume() override
  {
  }

  void
  send(const Block& wire) override;

  void
  send(const Block& header, const Block& payload) override;

private:
  NFD_LOG_INCLASS_DECLARE();

  signal::ScopedConnection m_fwToClientTransmitConn;
  signal::ScopedConnection m_clientToFwTransmitConn;
  signal::ScopedConnection m_fwTransportStateConn;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_INTERNAL_TRANSPORT_HPP
