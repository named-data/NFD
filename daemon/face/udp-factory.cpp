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

#include "udp-factory.hpp"
#include "generic-link-service.hpp"
#include "multicast-udp-transport.hpp"
#include "core/global-io.hpp"
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/copy.hpp>

#ifdef __linux__
#include <cerrno>       // for errno
#include <cstring>      // for std::strerror()
#include <sys/socket.h> // for setsockopt()
#endif // __linux__

namespace nfd {
namespace face {

namespace ip = boost::asio::ip;

NFD_LOG_INIT("UdpFactory");
NFD_REGISTER_PROTOCOL_FACTORY(UdpFactory);

const std::string&
UdpFactory::getId()
{
  static std::string id("udp");
  return id;
}


void
UdpFactory::processConfig(OptionalConfigSection configSection,
                          FaceSystem::ConfigContext& context)
{
  // udp
  // {
  //   port 6363
  //   enable_v4 yes
  //   enable_v6 yes
  //   idle_timeout 600
  //   keep_alive_interval 25 ; acceptable but ignored
  //   mcast yes
  //   mcast_group 224.0.23.170
  //   mcast_port 56363
  //   whitelist
  //   {
  //     *
  //   }
  //   blacklist
  //   {
  //   }
  // }

  uint16_t port = 6363;
  bool enableV4 = false;
  bool enableV6 = false;
  uint32_t idleTimeout = 600;
  MulticastConfig mcastConfig;

  if (configSection) {
    // These default to 'yes' but only if face_system.udp section is present
    enableV4 = enableV6 = mcastConfig.isEnabled = true;

    for (const auto& pair : *configSection) {
      const std::string& key = pair.first;
      const ConfigSection& value = pair.second;

      if (key == "port") {
        port = ConfigFile::parseNumber<uint16_t>(pair, "face_system.udp");
      }
      else if (key == "enable_v4") {
        enableV4 = ConfigFile::parseYesNo(pair, "face_system.udp");
      }
      else if (key == "enable_v6") {
        enableV6 = ConfigFile::parseYesNo(pair, "face_system.udp");
      }
      else if (key == "idle_timeout") {
        idleTimeout = ConfigFile::parseNumber<uint32_t>(pair, "face_system.udp");
      }
      else if (key == "keep_alive_interval") {
        // ignored
      }
      else if (key == "mcast") {
        mcastConfig.isEnabled = ConfigFile::parseYesNo(pair, "face_system.udp");
      }
      else if (key == "mcast_group") {
        const std::string& valueStr = value.get_value<std::string>();
        boost::system::error_code ec;
        mcastConfig.group.address(boost::asio::ip::address_v4::from_string(valueStr, ec));
        if (ec) {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("face_system.udp.mcast_group: '" +
                                valueStr + "' cannot be parsed as an IPv4 address"));
        }
        else if (!mcastConfig.group.address().is_multicast()) {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("face_system.udp.mcast_group: '" +
                                valueStr + "' is not a multicast address"));
        }
      }
      else if (key == "mcast_port") {
        mcastConfig.group.port(ConfigFile::parseNumber<uint16_t>(pair, "face_system.udp"));
      }
      else if (key == "whitelist") {
        mcastConfig.netifPredicate.parseWhitelist(value);
      }
      else if (key == "blacklist") {
        mcastConfig.netifPredicate.parseBlacklist(value);
      }
      else {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option face_system.udp." + key));
      }
    }

    if (!enableV4 && !enableV6 && !mcastConfig.isEnabled) {
      BOOST_THROW_EXCEPTION(ConfigFile::Error(
        "IPv4 and IPv6 UDP channels and UDP multicast have been disabled. "
        "Remove face_system.udp section to disable UDP channels or enable at least one of them."));
    }
  }

  if (!context.isDryRun) {
    if (enableV4) {
      udp::Endpoint endpoint(ip::udp::v4(), port);
      shared_ptr<UdpChannel> v4Channel = this->createChannel(endpoint, time::seconds(idleTimeout));
      if (!v4Channel->isListening()) {
        v4Channel->listen(context.addFace, nullptr);
      }
      providedSchemes.insert("udp");
      providedSchemes.insert("udp4");
    }
    else if (providedSchemes.count("udp4") > 0) {
      NFD_LOG_WARN("Cannot close udp4 channel after its creation");
    }

    if (enableV6) {
      udp::Endpoint endpoint(ip::udp::v6(), port);
      shared_ptr<UdpChannel> v6Channel = this->createChannel(endpoint, time::seconds(idleTimeout));
      if (!v6Channel->isListening()) {
        v6Channel->listen(context.addFace, nullptr);
      }
      providedSchemes.insert("udp");
      providedSchemes.insert("udp6");
    }
    else if (providedSchemes.count("udp6") > 0) {
      NFD_LOG_WARN("Cannot close udp6 channel after its creation");
    }

    if (m_mcastConfig.isEnabled != mcastConfig.isEnabled) {
      if (mcastConfig.isEnabled) {
        NFD_LOG_INFO("enabling multicast on " << mcastConfig.group);
      }
      else {
        NFD_LOG_INFO("disabling multicast");
      }
    }
    else if (m_mcastConfig.group != mcastConfig.group) {
      NFD_LOG_INFO("changing multicast group from " << m_mcastConfig.group <<
                   " to " << mcastConfig.group);
    }
    else if (m_mcastConfig.netifPredicate != mcastConfig.netifPredicate) {
      NFD_LOG_INFO("changing whitelist/blacklist");
    }
    else {
      // There's no configuration change, but we still need to re-apply configuration because
      // netifs may have changed.
    }

    m_mcastConfig = mcastConfig;
    this->applyMulticastConfig(context);
  }
}

void
UdpFactory::createFace(const FaceUri& uri,
                       ndn::nfd::FacePersistency persistency,
                       bool wantLocalFieldsEnabled,
                       const FaceCreatedCallback& onCreated,
                       const FaceCreationFailedCallback& onFailure)
{
  BOOST_ASSERT(uri.isCanonical());

  if (persistency == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND) {
    NFD_LOG_TRACE("createFace does not support FACE_PERSISTENCY_ON_DEMAND");
    onFailure(406, "Outgoing unicast UDP faces do not support on-demand persistency");
    return;
  }

  udp::Endpoint endpoint(ip::address::from_string(uri.getHost()),
                         boost::lexical_cast<uint16_t>(uri.getPort()));

  if (endpoint.address().is_multicast()) {
    NFD_LOG_TRACE("createFace does not support multicast faces");
    onFailure(406, "Cannot create multicast UDP faces");
    return;
  }

  if (m_prohibitedEndpoints.find(endpoint) != m_prohibitedEndpoints.end()) {
    NFD_LOG_TRACE("Requested endpoint is prohibited "
                  "(reserved by this NFD or disallowed by face management protocol)");
    onFailure(406, "Requested endpoint is prohibited");
    return;
  }

  if (wantLocalFieldsEnabled) {
    // UDP faces are never local
    NFD_LOG_TRACE("createFace cannot create non-local face with local fields enabled");
    onFailure(406, "Local fields can only be enabled on faces with local scope");
    return;
  }

  // very simple logic for now
  for (const auto& i : m_channels) {
    if ((i.first.address().is_v4() && endpoint.address().is_v4()) ||
        (i.first.address().is_v6() && endpoint.address().is_v6())) {
      i.second->connect(endpoint, persistency, onCreated, onFailure);
      return;
    }
  }

  NFD_LOG_TRACE("No channels available to connect to " + boost::lexical_cast<std::string>(endpoint));
  onFailure(504, "No channels available to connect");
}

void
UdpFactory::prohibitEndpoint(const udp::Endpoint& endpoint)
{
  if (endpoint.address().is_v4() &&
      endpoint.address() == ip::address_v4::any()) {
    prohibitAllIpv4Endpoints(endpoint.port());
  }
  else if (endpoint.address().is_v6() &&
           endpoint.address() == ip::address_v6::any()) {
    prohibitAllIpv6Endpoints(endpoint.port());
  }

  NFD_LOG_TRACE("prohibiting UDP " << endpoint);
  m_prohibitedEndpoints.insert(endpoint);
}

void
UdpFactory::prohibitAllIpv4Endpoints(uint16_t port)
{
  for (const NetworkInterfaceInfo& nic : listNetworkInterfaces()) {
    for (const auto& addr : nic.ipv4Addresses) {
      if (addr != ip::address_v4::any()) {
        prohibitEndpoint(udp::Endpoint(addr, port));
      }
    }

    if (nic.isBroadcastCapable() &&
        nic.broadcastAddress != ip::address_v4::any()) {
      prohibitEndpoint(udp::Endpoint(nic.broadcastAddress, port));
    }
  }

  prohibitEndpoint(udp::Endpoint(ip::address_v4::broadcast(), port));
}

void
UdpFactory::prohibitAllIpv6Endpoints(uint16_t port)
{
  for (const NetworkInterfaceInfo& nic : listNetworkInterfaces()) {
    for (const auto& addr : nic.ipv6Addresses) {
      if (addr != ip::address_v6::any()) {
        prohibitEndpoint(udp::Endpoint(addr, port));
      }
    }
  }
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const udp::Endpoint& endpoint,
                          const time::seconds& timeout)
{
  NFD_LOG_DEBUG("Creating unicast channel " << endpoint);

  auto channel = findChannel(endpoint);
  if (channel)
    return channel;

  if (endpoint.address().is_multicast()) {
    BOOST_THROW_EXCEPTION(Error("createChannel is only for unicast channels. The provided endpoint "
                                "is multicast. Use createMulticastFace to create a multicast face"));
  }

  // check if the endpoint is already used by a multicast face
  auto face = findMulticastFace(endpoint);
  if (face) {
    BOOST_THROW_EXCEPTION(Error("Cannot create the requested UDP unicast channel, local "
                                "endpoint is already allocated for a UDP multicast face"));
  }

  channel = std::make_shared<UdpChannel>(endpoint, timeout);
  m_channels[endpoint] = channel;
  prohibitEndpoint(endpoint);

  return channel;
}

shared_ptr<UdpChannel>
UdpFactory::createChannel(const std::string& localIp, const std::string& localPort,
                          const time::seconds& timeout)
{
  udp::Endpoint endpoint(ip::address::from_string(localIp),
                         boost::lexical_cast<uint16_t>(localPort));
  return createChannel(endpoint, timeout);
}

std::vector<shared_ptr<const Channel>>
UdpFactory::getChannels() const
{
  return getChannelsFromMap(m_channels);
}

shared_ptr<UdpChannel>
UdpFactory::findChannel(const udp::Endpoint& localEndpoint) const
{
  auto i = m_channels.find(localEndpoint);
  if (i != m_channels.end())
    return i->second;
  else
    return nullptr;
}

shared_ptr<Face>
UdpFactory::createMulticastFace(const udp::Endpoint& localEndpoint,
                                const udp::Endpoint& multicastEndpoint,
                                const std::string& networkInterfaceName/* = ""*/)
{
  // checking if the local and multicast endpoints are already in use for a multicast face
  auto face = findMulticastFace(localEndpoint);
  if (face) {
    if (face->getRemoteUri() == FaceUri(multicastEndpoint))
      return face;
    else
      BOOST_THROW_EXCEPTION(Error("Cannot create the requested UDP multicast face, local "
                                  "endpoint is already allocated for a UDP multicast face "
                                  "on a different multicast group"));
  }

  // checking if the local endpoint is already in use for a unicast channel
  auto unicastCh = findChannel(localEndpoint);
  if (unicastCh) {
    BOOST_THROW_EXCEPTION(Error("Cannot create the requested UDP multicast face, local "
                                "endpoint is already allocated for a UDP unicast channel"));
  }

  if (m_prohibitedEndpoints.find(multicastEndpoint) != m_prohibitedEndpoints.end()) {
    BOOST_THROW_EXCEPTION(Error("Cannot create the requested UDP multicast face, "
                                "remote endpoint is owned by this NFD instance"));
  }

  if (localEndpoint.address().is_v6() || multicastEndpoint.address().is_v6()) {
    BOOST_THROW_EXCEPTION(Error("IPv6 multicast is not supported yet. Please provide an IPv4 "
                                "address"));
  }

  if (localEndpoint.port() != multicastEndpoint.port()) {
    BOOST_THROW_EXCEPTION(Error("Cannot create the requested UDP multicast face, "
                                "both endpoints should have the same port number. "));
  }

  if (!multicastEndpoint.address().is_multicast()) {
    BOOST_THROW_EXCEPTION(Error("Cannot create the requested UDP multicast face, "
                                "the multicast group given as input is not a multicast address"));
  }

  ip::udp::socket receiveSocket(getGlobalIoService());
  receiveSocket.open(multicastEndpoint.protocol());
  receiveSocket.set_option(ip::udp::socket::reuse_address(true));
  receiveSocket.bind(multicastEndpoint);

  ip::udp::socket sendSocket(getGlobalIoService());
  sendSocket.open(multicastEndpoint.protocol());
  sendSocket.set_option(ip::udp::socket::reuse_address(true));
  sendSocket.set_option(ip::multicast::enable_loopback(false));
  sendSocket.bind(udp::Endpoint(ip::address_v4::any(), multicastEndpoint.port()));
  if (localEndpoint.address() != ip::address_v4::any())
    sendSocket.set_option(ip::multicast::outbound_interface(localEndpoint.address().to_v4()));

  sendSocket.set_option(ip::multicast::join_group(multicastEndpoint.address().to_v4(),
                                                  localEndpoint.address().to_v4()));
  receiveSocket.set_option(ip::multicast::join_group(multicastEndpoint.address().to_v4(),
                                                     localEndpoint.address().to_v4()));

#ifdef __linux__
  /*
   * On Linux, if there is more than one MulticastUdpFace for the same multicast
   * group but they are bound to different network interfaces, the socket needs
   * to be bound to the specific interface using SO_BINDTODEVICE, otherwise the
   * face will receive all packets sent to the other interfaces as well.
   * This happens only on Linux. On OS X, the ip::multicast::join_group option
   * is enough to get the desired behaviour.
   */
  if (!networkInterfaceName.empty()) {
    if (::setsockopt(receiveSocket.native_handle(), SOL_SOCKET, SO_BINDTODEVICE,
                     networkInterfaceName.c_str(), networkInterfaceName.size() + 1) < 0) {
      BOOST_THROW_EXCEPTION(Error("Cannot bind multicast face to " + networkInterfaceName +
                                  ": " + std::strerror(errno)));
    }
  }
#endif // __linux__

  auto linkService = make_unique<GenericLinkService>();
  auto transport = make_unique<MulticastUdpTransport>(localEndpoint, multicastEndpoint,
                                                      std::move(receiveSocket),
                                                      std::move(sendSocket));
  face = make_shared<Face>(std::move(linkService), std::move(transport));

  m_mcastFaces[localEndpoint] = face;
  connectFaceClosedSignal(*face, [this, localEndpoint] { m_mcastFaces.erase(localEndpoint); });

  return face;
}

shared_ptr<Face>
UdpFactory::createMulticastFace(const std::string& localIp,
                                const std::string& multicastIp,
                                const std::string& multicastPort,
                                const std::string& networkInterfaceName/* = ""*/)
{
  udp::Endpoint localEndpoint(ip::address::from_string(localIp),
                              boost::lexical_cast<uint16_t>(multicastPort));
  udp::Endpoint multicastEndpoint(ip::address::from_string(multicastIp),
                                  boost::lexical_cast<uint16_t>(multicastPort));
  return createMulticastFace(localEndpoint, multicastEndpoint, networkInterfaceName);
}

shared_ptr<Face>
UdpFactory::findMulticastFace(const udp::Endpoint& localEndpoint) const
{
  auto i = m_mcastFaces.find(localEndpoint);
  if (i != m_mcastFaces.end())
    return i->second;
  else
    return nullptr;
}

void
UdpFactory::applyMulticastConfig(const FaceSystem::ConfigContext& context)
{
  // collect old faces
  std::set<shared_ptr<Face>> oldFaces;
  boost::copy(m_mcastFaces | boost::adaptors::map_values,
              std::inserter(oldFaces, oldFaces.end()));

  if (m_mcastConfig.isEnabled) {
    // determine interfaces on which faces should be created or retained
    auto capableNetifRange = context.listNetifs() |
                             boost::adaptors::filtered([this] (const NetworkInterfaceInfo& netif) {
                               return netif.isUp() && netif.isMulticastCapable() &&
                                      !netif.ipv4Addresses.empty() &&
                                      m_mcastConfig.netifPredicate(netif);
                             });

    bool needIfname = false;
#ifdef __linux__
    std::vector<NetworkInterfaceInfo> capableNetifs;
    boost::copy(capableNetifRange, std::back_inserter(capableNetifs));
    // on Linux, ifname is needed to create more than one UDP multicast face on the same group
    needIfname = capableNetifs.size() > 1;
#else
    auto& capableNetifs = capableNetifRange;
#endif // __linux__

    // create faces
    for (const auto& netif : capableNetifs) {
      udp::Endpoint localEndpoint(netif.ipv4Addresses.front(), m_mcastConfig.group.port());
      shared_ptr<Face> face = this->createMulticastFace(localEndpoint, m_mcastConfig.group,
                                                        needIfname ? netif.name : "");
      if (face->getId() == INVALID_FACEID) {
        // new face: register with forwarding
        context.addFace(face);
      }
      else {
        // existing face: don't destroy
        oldFaces.erase(face);
      }
    }
  }

  // destroy old faces that are not needed in new configuration
  for (const auto& face : oldFaces) {
    face->close();
  }
}

} // namespace face
} // namespace nfd
