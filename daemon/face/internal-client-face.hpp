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

#ifndef NFD_DAEMON_FACE_INTERNAL_CLIENT_FACE_HPP
#define NFD_DAEMON_FACE_INTERNAL_CLIENT_FACE_HPP

#include "common.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/transport/transport.hpp>

namespace nfd {

class InternalFace;
class InternalClientFace;

shared_ptr<InternalClientFace>
makeInternalClientFace(const shared_ptr<InternalFace>& face, ndn::KeyChain& keyChain);

/** \brief a client face that is connected to NFD InternalFace
 */
class InternalClientFace : public ndn::Face
{
public:
  template<typename Packet>
  void
  receive(const Packet& packet);

private:
  class Transport : public ndn::Transport
  {
  public:
    signal::Signal<Transport, Block> onSendBlock;

    void
    receive(const Block& block);

    virtual void
    close() DECL_OVERRIDE;

    virtual void
    send(const Block& wire) DECL_OVERRIDE;

    virtual void
    send(const Block& header, const Block& payload) DECL_OVERRIDE;

    virtual void
    pause() DECL_OVERRIDE;

    virtual void
    resume() DECL_OVERRIDE;
  };

private:
  InternalClientFace(const shared_ptr<InternalFace>& face,
                     const shared_ptr<InternalClientFace::Transport>& transport,
                     ndn::KeyChain& keyChain);

  void
  construct();

  friend shared_ptr<InternalClientFace>
  makeInternalClientFace(const shared_ptr<InternalFace>& face,
                         ndn::KeyChain& keyChain);

private:
  shared_ptr<InternalFace> m_face;
  shared_ptr<InternalClientFace::Transport> m_transport;
};

template<typename Packet>
void
InternalClientFace::receive(const Packet& packet)
{
  if (!packet.getLocalControlHeader().empty(ndn::nfd::LocalControlHeader::ENCODE_ALL)) {

    Block header = packet.getLocalControlHeader()
                         .wireEncode(packet, ndn::nfd::LocalControlHeader::ENCODE_ALL);
    Block payload = packet.wireEncode();

    ndn::EncodingBuffer encoder(header.size() + payload.size(),
                                header.size() + payload.size());
    encoder.appendByteArray(header.wire(), header.size());
    encoder.appendByteArray(payload.wire(), payload.size());

    m_transport->receive(encoder.block());
  }
  else {
    m_transport->receive(packet.wireEncode());
  }
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_INTERNAL_CLIENT_FACE_HPP
