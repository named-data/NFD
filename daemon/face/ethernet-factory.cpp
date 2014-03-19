/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "ethernet-factory.hpp"
#include "core/global-io.hpp"
#include "core/network-interface.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <pcap/pcap.h>

namespace nfd {

NFD_LOG_INIT("EthernetFactory")

shared_ptr<EthernetFace>
EthernetFactory::createMulticastFace(const shared_ptr<NetworkInterfaceInfo> &interface,
                                     const ethernet::Address &address)
{
  if (!address.isMulticast())
    throw Error(address.toString() + " is not a multicast address");

  const std::string& name = interface->name;
  shared_ptr<EthernetFace> face = findMulticastFace(name, address);
  if (face)
    return face;

  shared_ptr<boost::asio::posix::stream_descriptor> socket =
    make_shared<boost::asio::posix::stream_descriptor>(boost::ref(getGlobalIoService()));

  face = make_shared<EthernetFace>(boost::cref(socket),
                                   boost::cref(interface),
                                   boost::cref(address));
  face->onFail += bind(&EthernetFactory::afterFaceFailed,
                       this, name, address);
  m_multicastFaces[std::make_pair(name, address)] = face;

  return face;
}

void
EthernetFactory::afterFaceFailed(const std::string& interfaceName,
                                 const ethernet::Address& address)
{
  NFD_LOG_DEBUG("afterFaceFailed: " << interfaceName << "/" << address);
  m_multicastFaces.erase(std::make_pair(interfaceName, address));
}

shared_ptr<EthernetFace>
EthernetFactory::findMulticastFace(const std::string& interfaceName,
                                   const ethernet::Address& address) const
{
  MulticastFacesMap::const_iterator i = m_multicastFaces.find(std::make_pair(interfaceName, address));
  if (i != m_multicastFaces.end())
    return i->second;
  else
    return shared_ptr<EthernetFace>();
}

void
EthernetFactory::createFace(const FaceUri& uri,
                            const FaceCreatedCallback& onCreated,
                            const FaceConnectFailedCallback& onConnectFailed)
{
  throw Error("EthernetFactory does not support 'createFace' operation");
}

} // namespace nfd
