/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "ethernet-factory.hpp"
#include "ethernet-transport.hpp"
#include "generic-link-service.hpp"

namespace nfd {

shared_ptr<Face>
EthernetFactory::createMulticastFace(const NetworkInterfaceInfo& interface,
                                     const ethernet::Address& address)
{
  if (!address.isMulticast())
    BOOST_THROW_EXCEPTION(Error(address.toString() + " is not a multicast address"));

  auto face = findMulticastFace(interface.name, address);
  if (face)
    return face;

  face::GenericLinkService::Options opts;
  opts.allowFragmentation = true;
  opts.allowReassembly = true;

  auto linkService = make_unique<face::GenericLinkService>(opts);
  auto transport = make_unique<face::EthernetTransport>(interface, address);
  face = make_shared<Face>(std::move(linkService), std::move(transport));

  auto key = std::make_pair(interface.name, address);
  m_multicastFaces[key] = face;
  connectFaceClosedSignal(*face, [this, key] { m_multicastFaces.erase(key); });

  return face;
}

void
EthernetFactory::createFace(const FaceUri& uri,
                            ndn::nfd::FacePersistency persistency,
                            bool wantLocalFieldsEnabled,
                            const FaceCreatedCallback& onCreated,
                            const FaceCreationFailedCallback& onFailure)
{
  onFailure(406, "Unsupported protocol");
}

std::vector<shared_ptr<const Channel>>
EthernetFactory::getChannels() const
{
  return {};
}

shared_ptr<Face>
EthernetFactory::findMulticastFace(const std::string& interfaceName,
                                   const ethernet::Address& address) const
{
  auto i = m_multicastFaces.find({interfaceName, address});
  if (i != m_multicastFaces.end())
    return i->second;
  else
    return nullptr;
}

} // namespace nfd
