/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#ifndef NFD_DAEMON_FACE_ETHERNET_FACTORY_HPP
#define NFD_DAEMON_FACE_ETHERNET_FACTORY_HPP

#include "protocol-factory.hpp"

namespace nfd {
namespace face {

/** \brief protocol factory for Ethernet
 *
 *  Currently, only Ethernet multicast is supported.
 */
class EthernetFactory : public ProtocolFactory
{
public:
  static const std::string&
  getId();

  /** \brief process face_system.ether config section
   */
  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) override;

  /** \brief unicast face creation is not supported and will always fail
   */
  void
  createFace(const FaceUri& uri,
             ndn::nfd::FacePersistency persistency,
             bool wantLocalFieldsEnabled,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) override;

  /** \return empty container, because Ethernet unicast is not supported
   */
  std::vector<shared_ptr<const Channel>>
  getChannels() const override;

private:
  /** \brief Create a face to communicate on the given Ethernet multicast group
   *  \param netif local network interface
   *  \param group multicast group address
   *  \note Calling this method again with same arguments returns the existing face on the given
   *        interface and multicast group rather than creating a new one.
   *  \throw EthernetTransport::Error transport creation fails
   */
  shared_ptr<Face>
  createMulticastFace(const NetworkInterfaceInfo& netif, const ethernet::Address& group);

  void
  applyConfig(const FaceSystem::ConfigContext& context);

private:
  struct MulticastConfig
  {
    bool isEnabled = false;
    ethernet::Address group = ethernet::getDefaultMulticastAddress();
    NetworkInterfacePredicate netifPredicate;
  };

  MulticastConfig m_mcastConfig;

  /// (ifname, group) => face
  std::map<std::pair<std::string, ethernet::Address>, shared_ptr<Face>> m_mcastFaces;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_ETHERNET_FACTORY_HPP
