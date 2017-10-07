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

#ifndef NFD_DAEMON_FACE_UDP_FACTORY_HPP
#define NFD_DAEMON_FACE_UDP_FACTORY_HPP

#include "protocol-factory.hpp"
#include "udp-channel.hpp"

namespace nfd {
namespace face {

/** \brief protocol factory for UDP over IPv4 and IPv6
 *
 *  UDP unicast is available over both IPv4 and IPv6.
 *  UDP multicast is available over IPv4 only.
 */
class UdpFactory : public ProtocolFactory
{
public:
  /**
   * \brief Exception of UdpFactory
   */
  class Error : public ProtocolFactory::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : ProtocolFactory::Error(what)
    {
    }
  };

  static const std::string&
  getId();

  explicit
  UdpFactory(const CtorParams& params);

  /** \brief process face_system.udp config section
   */
  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) override;

  void
  createFace(const CreateFaceParams& params,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) override;

  /**
   * \brief Create UDP-based channel using udp::Endpoint
   *
   * udp::Endpoint is really an alias for boost::asio::ip::udp::endpoint.
   *
   * If this method is called twice with the same endpoint, only one channel
   * will be created. The second call will just return the existing channel.
   *
   * If a multicast face is already active on the same local endpoint,
   * the creation fails and an exception is thrown.
   *
   * \return always a valid pointer to a UdpChannel object, an exception
   *         is thrown if it cannot be created.
   * \throw UdpFactory::Error
   */
  shared_ptr<UdpChannel>
  createChannel(const udp::Endpoint& localEndpoint,
                time::nanoseconds idleTimeout);

  std::vector<shared_ptr<const Channel>>
  getChannels() const override;

  /**
   * \brief Create multicast UDP face using udp::Endpoint
   *
   * udp::Endpoint is really an alias for boost::asio::ip::udp::endpoint.
   *
   * The face will join the specified multicast group.
   *
   * If this method is called twice with the same pair of endpoints, only one
   * face will be created. The second call will just return the existing face.
   *
   * If an unicast face is already active on the same local NIC and port, the
   * creation fails and an exception is thrown.
   *
   * \param localEndpoint local endpoint
   * \param multicastEndpoint multicast endpoint
   * \param networkInterfaceName name of the network interface on which the face will be bound
   *        (Used only on multihomed linux machine with more than one multicast UDP face for
   *        the same multicast group. If specified, will require CAP_NET_RAW capability)
   *        An empty string can be provided in other system or in linux machine with only one
   *        multicast UDP face per multicast group
   *
   * \return always a valid pointer to a MulticastUdpFace object, an exception
   *         is thrown if it cannot be created.
   * \throw UdpFactory::Error
   */
  shared_ptr<Face>
  createMulticastFace(const udp::Endpoint& localEndpoint,
                      const udp::Endpoint& multicastEndpoint,
                      const std::string& networkInterfaceName = "");

  shared_ptr<Face>
  createMulticastFace(const std::string& localIp,
                      const std::string& multicastIp,
                      const std::string& multicastPort,
                      const std::string& networkInterfaceName = "");

private:
  /** \brief Create UDP multicast face on \p netif if needed by \p m_mcastConfig.
   *  \return new or existing face, or nullptr if no face should be created
   */
  shared_ptr<Face>
  applyMcastConfigToNetif(const shared_ptr<const ndn::net::NetworkInterface>& netif);

  /** \brief Create and destroy UDP multicast faces according to \p m_mcastConfig.
   */
  void
  applyMcastConfig(const FaceSystem::ConfigContext& context);

private:
  std::map<udp::Endpoint, shared_ptr<UdpChannel>> m_channels;

  struct MulticastConfig
  {
    bool isEnabled = false;
    udp::Endpoint group = udp::getDefaultMulticastGroup();
    ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_MULTI_ACCESS;
    NetworkInterfacePredicate netifPredicate;
  };

  MulticastConfig m_mcastConfig;
  std::map<udp::Endpoint, shared_ptr<Face>> m_mcastFaces;

  signal::ScopedConnection m_netifAddConn;
  struct NetifConns
  {
    signal::ScopedConnection addrAddConn;
  };
  std::map<int, NetifConns> m_netifConns; // ifindex => signal connections
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_UDP_FACTORY_HPP
