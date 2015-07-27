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

#include "ethernet-factory.hpp"
#include "face/ethernet-face.hpp"

#include "core/logger.hpp"
#include "core/global-io.hpp"

namespace nfd {

shared_ptr<EthernetFace>
EthernetFactory::createMulticastFace(const NetworkInterfaceInfo& interface,
                                     const ethernet::Address &address)
{
  if (!address.isMulticast())
    BOOST_THROW_EXCEPTION(Error(address.toString() + " is not a multicast address"));

  shared_ptr<EthernetFace> face = findMulticastFace(interface.name, address);
  if (face)
    return face;

  face = make_shared<EthernetFace>(boost::asio::posix::stream_descriptor(getGlobalIoService()),
                                   interface, address);

  auto key = std::make_pair(interface.name, address);
  face->onFail.connectSingleShot([this, key] (const std::string& reason) {
    m_multicastFaces.erase(key);
  });
  m_multicastFaces.insert({key, face});

  return face;
}

shared_ptr<EthernetFace>
EthernetFactory::findMulticastFace(const std::string& interfaceName,
                                   const ethernet::Address& address) const
{
  auto it = m_multicastFaces.find({interfaceName, address});
  if (it != m_multicastFaces.end())
    return it->second;
  else
    return {};
}

void
EthernetFactory::createFace(const FaceUri& uri,
                            ndn::nfd::FacePersistency persistency,
                            const FaceCreatedCallback& onCreated,
                            const FaceConnectFailedCallback& onConnectFailed)
{
  BOOST_THROW_EXCEPTION(Error("EthernetFactory does not support 'createFace' operation"));
}

std::list<shared_ptr<const Channel>>
EthernetFactory::getChannels() const
{
  return {};
}

} // namespace nfd
