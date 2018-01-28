/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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
#include "ethernet-channel.hpp"

namespace nfd {
namespace face {

/** \brief protocol factory for Ethernet
 */
class EthernetFactory : public ProtocolFactory
{
public:
  static const std::string&
  getId();

  explicit
  EthernetFactory(const CtorParams& params);

  /** \brief process face_system.ether config section
   */
  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) override;

  void
  createFace(const CreateFaceRequest& req,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) override;

  /**
   * \brief Create Ethernet-based channel on the specified network interface
   *
   * If this method is called twice with the same endpoint, only one channel
   * will be created. The second call will just return the existing channel.
   *
   * \return always a valid pointer to a EthernetChannel object, an exception
   *         is thrown if it cannot be created.
   * \throw PcapHelper::Error channel creation failed
   */
  shared_ptr<EthernetChannel>
  createChannel(const shared_ptr<const ndn::net::NetworkInterface>& localEndpoint,
                time::nanoseconds idleTimeout);

  std::vector<shared_ptr<const Channel>>
  getChannels() const override;

  /**
   * \brief Create a face to communicate on the given Ethernet multicast group
   *
   * If this method is called twice with the same arguments, only one face
   * will be created. The second call will just return the existing face.
   *
   * \param localEndpoint local network interface
   * \param group multicast group address
   *
   * \throw EthernetTransport::Error transport creation fails
   */
  shared_ptr<Face>
  createMulticastFace(const ndn::net::NetworkInterface& localEndpoint,
                      const ethernet::Address& group);

private:
  /** \brief Create EthernetChannel on \p netif if requested by \p m_unicastConfig.
   *  \return new or existing channel, or nullptr if no channel should be created
   */
  shared_ptr<EthernetChannel>
  applyUnicastConfigToNetif(const shared_ptr<const ndn::net::NetworkInterface>& netif);

  /** \brief Create Ethernet multicast face on \p netif if requested by \p m_mcastConfig.
   *  \return new or existing face, or nullptr if no face should be created
   */
  shared_ptr<Face>
  applyMcastConfigToNetif(const ndn::net::NetworkInterface& netif);

  void
  applyConfig(const FaceSystem::ConfigContext& context);

private:
  std::map<std::string, shared_ptr<EthernetChannel>> m_channels; ///< ifname => channel

  struct UnicastConfig
  {
    bool isEnabled = false;
    bool wantListen = false;
    time::nanoseconds idleTimeout = 10_min;
  };
  UnicastConfig m_unicastConfig;

  struct MulticastConfig
  {
    bool isEnabled = false;
    ethernet::Address group = ethernet::getDefaultMulticastAddress();
    ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_MULTI_ACCESS;
    NetworkInterfacePredicate netifPredicate;
  };
  MulticastConfig m_mcastConfig;

  /// (ifname, group) => face
  std::map<std::pair<std::string, ethernet::Address>, shared_ptr<Face>> m_mcastFaces;

  signal::ScopedConnection m_netifAddConn;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_ETHERNET_FACTORY_HPP
