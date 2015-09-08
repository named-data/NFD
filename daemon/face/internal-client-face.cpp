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

#include "internal-client-face.hpp"
#include "internal-face.hpp"
#include "core/global-io.hpp"

#include <ndn-cxx/management/nfd-local-control-header.hpp>

namespace nfd {

void
InternalClientFace::Transport::receive(const Block& block)
{
  if (m_receiveCallback) {
    m_receiveCallback(block);
  }
}

void
InternalClientFace::Transport::close()
{
}

void
InternalClientFace::Transport::send(const Block& wire)
{
  onSendBlock(wire);
}

void
InternalClientFace::Transport::send(const Block& header, const Block& payload)
{
  ndn::EncodingBuffer encoder(header.size() + payload.size(), header.size() + payload.size());
  encoder.appendByteArray(header.wire(), header.size());
  encoder.appendByteArray(payload.wire(), payload.size());

  this->send(encoder.block());
}

void
InternalClientFace::Transport::pause()
{
}

void
InternalClientFace::Transport::resume()
{
}

InternalClientFace::InternalClientFace(const shared_ptr<InternalFace>& face,
                                       const shared_ptr<InternalClientFace::Transport>& transport,
                                       ndn::KeyChain& keyChain)
  : ndn::Face(transport, getGlobalIoService(), keyChain)
  , m_face(face)
  , m_transport(transport)
{
  construct();
}

void
InternalClientFace::construct()
{
  m_face->onSendInterest.connect(bind(&InternalClientFace::receive<Interest>, this, _1));
  m_face->onSendData.connect(bind(&InternalClientFace::receive<Data>, this, _1));
  m_transport->onSendBlock.connect(bind(&InternalFace::receive, m_face, _1));
}

shared_ptr<InternalClientFace>
makeInternalClientFace(const shared_ptr<InternalFace>& face,
                       ndn::KeyChain& keyChain)
{
  auto transport = make_shared<InternalClientFace::Transport>();
  return shared_ptr<InternalClientFace>(new InternalClientFace(face, transport, keyChain));
}

} // namespace nfd
