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

#ifndef NFD_DAEMON_FACE_ETHERNET_FACTORY_HPP
#define NFD_DAEMON_FACE_ETHERNET_FACTORY_HPP

#include "protocol-factory.hpp"
#include "core/network-interface.hpp"

namespace nfd {

class EthernetFace;

class EthernetFactory : public ProtocolFactory
{
public:
  /**
   * \brief Exception of EthernetFactory
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

  typedef std::map<std::pair<std::string, ethernet::Address>,
                   shared_ptr<EthernetFace>> MulticastFaceMap;

  // from ProtocolFactory
  virtual void
  createFace(const FaceUri& uri,
             ndn::nfd::FacePersistency persistency,
             const FaceCreatedCallback& onCreated,
             const FaceConnectFailedCallback& onConnectFailed) DECL_OVERRIDE;

  /**
   * \brief Create an EthernetFace to communicate with the given multicast group
   *
   * If this method is called twice with the same interface and group, only
   * one face will be created. Instead, the second call will just retrieve
   * the existing face.
   *
   * \param interface Local network interface
   * \param address   Ethernet broadcast/multicast destination address
   *
   * \returns always a valid shared pointer to an EthernetFace object,
   *          an exception will be thrown if the creation fails
   *
   * \throws EthernetFactory::Error or EthernetFace::Error
   */
  shared_ptr<EthernetFace>
  createMulticastFace(const NetworkInterfaceInfo& interface,
                      const ethernet::Address& address);

  /**
   * \brief Get map of configured multicast faces
   */
  const MulticastFaceMap&
  getMulticastFaces() const;

  virtual std::list<shared_ptr<const Channel>>
  getChannels() const;

private:
  /**
   * \brief Look up EthernetFace using specified interface and address
   *
   * \returns shared pointer to the existing EthernetFace object or
   *          empty shared pointer when such face does not exist
   *
   * \throws never
   */
  shared_ptr<EthernetFace>
  findMulticastFace(const std::string& interfaceName,
                    const ethernet::Address& address) const;

private:
  MulticastFaceMap m_multicastFaces;
};

inline const EthernetFactory::MulticastFaceMap&
EthernetFactory::getMulticastFaces() const
{
  return m_multicastFaces;
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_ETHERNET_FACTORY_HPP
