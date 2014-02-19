/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_ETHERNET_CHANNEL_FACTORY_HPP
#define NFD_FACE_ETHERNET_CHANNEL_FACTORY_HPP

#include "ethernet-face.hpp"

namespace nfd {

class EthernetChannelFactory
{
public:
  /**
   * \brief Exception of EthernetChannelFactory
   */
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };

  /**
   * \brief Create an EthernetFace to communicate with the given multicast group
   *
   * If this method is called twice with the same endpoint and group, only
   * one face will be created. Instead, the second call will just retrieve
   * the existing face.
   *
   * \param interface Local network interface
   * \param address   Ethernet broadcast/multicast destination address
   *
   * \returns always a valid shared pointer to an EthernetFace object,
   *          an exception will be thrown if the creation fails
   *
   * \throws EthernetChannelFactory::Error or EthernetFace::Error
   */
  shared_ptr<EthernetFace>
  createMulticast(const ethernet::Endpoint& interface,
                  const ethernet::Address& address);

  /**
   * \brief Get a list of devices that can be opened for a live capture
   *
   * This function is a wrapper for pcap_findalldevs()/pcap_freealldevs()
   */
  static std::vector<ethernet::Endpoint>
  findAllInterfaces();

private:
  void
  afterFaceFailed(const ethernet::Endpoint& endpoint,
                  const ethernet::Address& address);

  /**
   * \brief Look up EthernetFace using specified interface and address
   *
   * \returns shared pointer to the existing EthernetFace object or
   *          empty shared pointer when such face does not exist
   *
   * \throws never
   */
  shared_ptr<EthernetFace>
  findMulticast(const ethernet::Endpoint& interface,
                const ethernet::Address& address) const;

private:
  typedef std::map< std::pair<ethernet::Endpoint, ethernet::Address>,
                    shared_ptr<EthernetFace> > MulticastFacesMap;
  MulticastFacesMap m_multicastFaces;
};

} // namespace nfd

#endif // NFD_FACE_ETHERNET_CHANNEL_FACTORY_HPP
