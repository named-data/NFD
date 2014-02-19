/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "ethernet-channel-factory.hpp"
#include "core/global-io.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <pcap/pcap.h>

namespace nfd {

NFD_LOG_INIT("EthernetChannelFactory")

shared_ptr<EthernetFace>
EthernetChannelFactory::createMulticast(const ethernet::Endpoint& interface,
                                        const ethernet::Address& address)
{
  std::vector<ethernet::Endpoint> ifs = findAllInterfaces();
  if (std::find(ifs.begin(), ifs.end(), interface) == ifs.end())
    throw Error(interface + " does not exist");

  if (!address.isMulticast())
    throw Error(address.toString() + " is not a multicast address");

  shared_ptr<EthernetFace> face = findMulticast(interface, address);
  if (face)
    return face;

  shared_ptr<boost::asio::posix::stream_descriptor> socket =
    make_shared<boost::asio::posix::stream_descriptor>(boost::ref(getGlobalIoService()));

  face = make_shared<EthernetFace>(boost::cref(socket),
                                   boost::cref(interface),
                                   boost::cref(address));
  face->onFail += bind(&EthernetChannelFactory::afterFaceFailed,
                       this, interface, address);
  m_multicastFaces[std::make_pair(interface, address)] = face;

  return face;
}

std::vector<ethernet::Endpoint>
EthernetChannelFactory::findAllInterfaces()
{
  std::vector<ethernet::Endpoint> interfaces;
  char errbuf[PCAP_ERRBUF_SIZE];
  errbuf[0] = '\0';

  pcap_if_t* alldevs;
  if (pcap_findalldevs(&alldevs, errbuf) < 0)
    {
      NFD_LOG_WARN("pcap_findalldevs() failed: " << errbuf);
      return interfaces;
    }

  for (pcap_if_t* device = alldevs; device != 0; device = device->next)
    {
      if ((device->flags & PCAP_IF_LOOPBACK) != 0)
        // ignore loopback devices
        continue;

      ethernet::Endpoint interface(device->name);
      if (interface == "any")
        // ignore libpcap "any" pseudo-device
        continue;
      if (boost::starts_with(interface, "nflog") ||
          boost::starts_with(interface, "nfqueue"))
        // ignore Linux netfilter devices
        continue;

      // maybe add interface addresses too
      // interface.addAddress ...
      interfaces.push_back(interface);
    }

  pcap_freealldevs(alldevs);
  return interfaces;
}

void
EthernetChannelFactory::afterFaceFailed(const ethernet::Endpoint& interface,
                                        const ethernet::Address& address)
{
  NFD_LOG_DEBUG("afterFaceFailed: " << interface << "/" << address);
  m_multicastFaces.erase(std::make_pair(interface, address));
}

shared_ptr<EthernetFace>
EthernetChannelFactory::findMulticast(const ethernet::Endpoint& interface,
                                      const ethernet::Address& address) const
{
  MulticastFacesMap::const_iterator i = m_multicastFaces.find(std::make_pair(interface, address));
  if (i != m_multicastFaces.end())
    return i->second;
  else
    return shared_ptr<EthernetFace>();
}

} // namespace nfd
