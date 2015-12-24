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

#include "internal-face.hpp"
#include "generic-link-service.hpp"
#include "internal-transport.hpp"
#include "core/global-io.hpp"

namespace nfd {
namespace face {

std::tuple<shared_ptr<Face>, shared_ptr<ndn::Face>>
makeInternalFace(ndn::KeyChain& clientKeyChain)
{
  GenericLinkService::Options serviceOpts;
  serviceOpts.allowLocalFields = true;

  auto face = make_shared<Face>(make_unique<GenericLinkService>(serviceOpts),
                                make_unique<InternalForwarderTransport>());

  auto forwarderTransport = static_cast<InternalForwarderTransport*>(face->getTransport());
  auto clientTransport = make_shared<InternalClientTransport>();
  clientTransport->connectToForwarder(forwarderTransport);

  auto clientFace = make_shared<ndn::Face>(clientTransport, getGlobalIoService(), clientKeyChain);

  return std::make_tuple(face, clientFace);
}

} // namespace face
} // namespace nfd
