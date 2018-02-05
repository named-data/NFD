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

#include "face/udp-factory.hpp"

#include "face-system-fixture.hpp"
#include "factory-test-common.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <ndn-cxx/net/address-converter.hpp>

namespace nfd {
namespace face {
namespace tests {

class UdpFactoryFixture : public FaceSystemFactoryFixture<UdpFactory>
{
protected:
  shared_ptr<UdpChannel>
  createChannel(const std::string& localIp, uint16_t localPort)
  {
    udp::Endpoint endpoint(ndn::ip::addressFromString(localIp), localPort);
    return factory.createChannel(endpoint, 5_min);
  }
};

class UdpFactoryMcastFixture : public UdpFactoryFixture
{
protected:
  UdpFactoryMcastFixture()
  {
    for (const auto& netif : collectNetworkInterfaces()) {
      // same filtering logic as UdpFactory::applyMcastConfigToNetif()
      if (netif->isUp() && !netif->isLoopback() && netif->canMulticast()) {
        bool hasValidIpAddress = false;
        if (hasAddressFamily(*netif, ndn::net::AddressFamily::V4)) {
          hasValidIpAddress = true;
          netifsV4.push_back(netif);
        }
        if (hasAddressFamily(*netif, ndn::net::AddressFamily::V6)) {
          hasValidIpAddress = true;
          netifsV6.push_back(netif);
        }
        if (hasValidIpAddress) {
          netifs.push_back(netif);
        }
      }
    }
    this->copyRealNetifsToNetmon();
  }

  shared_ptr<Face>
  createMulticastFace(const std::string& localIp, const std::string& mcastIp, uint16_t mcastPort)
  {
    auto localAddress = ndn::ip::addressFromString(localIp);
    udp::Endpoint mcastEndpoint(ndn::ip::addressFromString(mcastIp), mcastPort);

    if (localAddress.is_v4()) {
      BOOST_ASSERT(!netifsV4.empty());
      return factory.createMulticastFace(netifsV4.front(), localAddress, mcastEndpoint);
    }
    else {
      BOOST_ASSERT(!netifsV6.empty());
      return factory.createMulticastFace(netifsV6.front(), localAddress, mcastEndpoint);
    }
  }

  /** \brief returns a non-loopback IP address suitable for the creation of a UDP multicast face
   */
  boost::asio::ip::address
  findNonLoopbackAddressForMulticastFace(ndn::net::AddressFamily af) const
  {
    const auto& netifList = af == ndn::net::AddressFamily::V4 ? netifsV4 : netifsV6;
    for (const auto& netif : netifList) {
      for (const auto& a : netif->getNetworkAddresses()) {
        if (a.getFamily() == af && !a.getIp().is_loopback())
          return a.getIp();
      }
    }
    return {};
  }

  std::vector<const Face*>
  listUdp4McastFaces(ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_MULTI_ACCESS) const
  {
    return this->listFacesByScheme("udp4", linkType);
  }

  std::vector<const Face*>
  listUdp6McastFaces(ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_MULTI_ACCESS) const
  {
    return this->listFacesByScheme("udp6", linkType);
  }

  /** \brief determine whether \p netif has at least one IP address of the given family
   */
  static bool
  hasAddressFamily(const NetworkInterface& netif, ndn::net::AddressFamily af)
  {
    return std::any_of(netif.getNetworkAddresses().begin(), netif.getNetworkAddresses().end(),
                       [af] (const NetworkAddress& a) { return a.getFamily() == af; });
  }

  /** \brief determine whether a UDP multicast face is created on \p netif
   */
  static bool
  isFaceOnNetif(const Face& face, const NetworkInterface& netif)
  {
    auto ip = ndn::ip::addressFromString(face.getLocalUri().getHost());
    return std::any_of(netif.getNetworkAddresses().begin(), netif.getNetworkAddresses().end(),
                       [ip] (const NetworkAddress& a) { return a.getIp() == ip; });
  }

protected:
  /** \brief MulticastUdpTransport-capable network interfaces (IPv4 + IPv6)
   *
   *  This should be used in test cases that do not depend on a specific address family
   */
  std::vector<shared_ptr<const NetworkInterface>> netifs;

  /** \brief MulticastUdpTransport-capable network interfaces (IPv4 only)
   */
  std::vector<shared_ptr<const NetworkInterface>> netifsV4;

  /** \brief MulticastUdpTransport-capable network interfaces (IPv6 only)
   */
  std::vector<shared_ptr<const NetworkInterface>> netifsV6;
};

#define SKIP_IF_UDP_MCAST_NETIF_COUNT_LT(n) \
  do { \
    if (this->netifs.size() < (n)) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require " #n \
                                " or more MulticastUdpTransport-capable network interfaces"); \
      return; \
    } \
  } while (false)

#define SKIP_IF_UDP_MCAST_V4_NETIF_COUNT_LT(n) \
  do { \
    if (this->netifsV4.size() < (n)) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require " #n \
                                " or more IPv4 MulticastUdpTransport-capable network interfaces"); \
      return; \
    } \
  } while (false)

#define SKIP_IF_UDP_MCAST_V6_NETIF_COUNT_LT(n) \
  do { \
    if (this->netifsV6.size() < (n)) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require " #n \
                                " or more IPv6 MulticastUdpTransport-capable network interfaces"); \
      return; \
    } \
  } while (false)

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestUdpFactory, UdpFactoryFixture)

BOOST_AUTO_TEST_SUITE(ProcessConfig)

using nfd::Face;

BOOST_AUTO_TEST_CASE(Defaults)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"udp4://0.0.0.0:6363", "udp6://[::]:6363"});
  auto channels = factory.getChannels();
  BOOST_CHECK(std::all_of(channels.begin(), channels.end(),
                          [] (const shared_ptr<const Channel>& ch) { return ch->isListening(); }));
}

BOOST_AUTO_TEST_CASE(DisableListen)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        listen no
        port 7001
        mcast no
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"udp4://0.0.0.0:7001", "udp6://[::]:7001"});
  auto channels = factory.getChannels();
  BOOST_CHECK(std::none_of(channels.begin(), channels.end(),
                           [] (const shared_ptr<const Channel>& ch) { return ch->isListening(); }));
}

BOOST_AUTO_TEST_CASE(DisableV4)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        port 7001
        enable_v4 no
        enable_v6 yes
        mcast no
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"udp6://[::]:7001"});
}

BOOST_AUTO_TEST_CASE(DisableV6)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        port 7001
        enable_v4 yes
        enable_v6 no
        mcast no
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"udp4://0.0.0.0:7001"});
}

BOOST_FIXTURE_TEST_CASE(EnableDisableMcast, UdpFactoryMcastFixture)
{
  const std::string CONFIG_WITH_MCAST = R"CONFIG(
    face_system
    {
      udp
      {
        mcast yes
      }
    }
  )CONFIG";
  const std::string CONFIG_WITHOUT_MCAST = R"CONFIG(
    face_system
    {
      udp
      {
        mcast no
      }
    }
  )CONFIG";

  parseConfig(CONFIG_WITHOUT_MCAST, false);
  BOOST_CHECK_EQUAL(this->listUdp4McastFaces().size(), 0);
  BOOST_CHECK_EQUAL(this->listUdp6McastFaces().size(), 0);

#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__

  parseConfig(CONFIG_WITH_MCAST, false);
  g_io.poll();
  BOOST_CHECK_EQUAL(this->listUdp4McastFaces().size(), netifsV4.size());
  BOOST_CHECK_EQUAL(this->listUdp6McastFaces().size(), netifsV6.size());

  parseConfig(CONFIG_WITHOUT_MCAST, false);
  g_io.poll();
  BOOST_CHECK_EQUAL(this->listUdp4McastFaces().size(), 0);
  BOOST_CHECK_EQUAL(this->listUdp6McastFaces().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(McastAdHoc, UdpFactoryMcastFixture)
{
#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__
  SKIP_IF_UDP_MCAST_NETIF_COUNT_LT(1);

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_ad_hoc yes
      }
    }
  )CONFIG";

  parseConfig(CONFIG, false);
  BOOST_CHECK_EQUAL(this->listUdp4McastFaces(ndn::nfd::LINK_TYPE_AD_HOC).size(), netifsV4.size());
  BOOST_CHECK_EQUAL(this->listUdp6McastFaces(ndn::nfd::LINK_TYPE_AD_HOC).size(), netifsV6.size());
}

BOOST_FIXTURE_TEST_CASE(ChangeMcastEndpointV4, UdpFactoryMcastFixture)
{
#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__
  SKIP_IF_UDP_MCAST_V4_NETIF_COUNT_LT(1);

  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group 239.66.30.1
        mcast_port 7011
      }
    }
  )CONFIG";
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group 239.66.30.2
        mcast_port 7012
      }
    }
  )CONFIG";

  parseConfig(CONFIG1, false);
  auto udpMcastFaces = this->listUdp4McastFaces();
  BOOST_REQUIRE_EQUAL(udpMcastFaces.size(), netifsV4.size());
  BOOST_CHECK_EQUAL(udpMcastFaces.front()->getRemoteUri(), FaceUri("udp4://239.66.30.1:7011"));

  parseConfig(CONFIG2, false);
  g_io.poll();
  udpMcastFaces = this->listUdp4McastFaces();
  BOOST_REQUIRE_EQUAL(udpMcastFaces.size(), netifsV4.size());
  BOOST_CHECK_EQUAL(udpMcastFaces.front()->getRemoteUri(), FaceUri("udp4://239.66.30.2:7012"));
}

BOOST_FIXTURE_TEST_CASE(ChangeMcastEndpointV6, UdpFactoryMcastFixture)
{
#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__
  SKIP_IF_UDP_MCAST_V6_NETIF_COUNT_LT(1);

  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group_v6 ff02::1101
        mcast_port_v6 7011
      }
    }
  )CONFIG";
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group_v6 ff02::1102
        mcast_port_v6 7012
      }
    }
  )CONFIG";

  parseConfig(CONFIG1, false);
  auto udpMcastFaces = this->listUdp6McastFaces();
  BOOST_REQUIRE_EQUAL(udpMcastFaces.size(), netifsV6.size());
  auto expectedAddr = boost::asio::ip::address_v6::from_string("ff02::1101");
  expectedAddr.scope_id(netifsV6.front()->getIndex());
  BOOST_CHECK_EQUAL(udpMcastFaces.front()->getRemoteUri(), FaceUri(udp::Endpoint(expectedAddr, 7011)));

  parseConfig(CONFIG2, false);
  g_io.poll();
  udpMcastFaces = this->listUdp6McastFaces();
  BOOST_REQUIRE_EQUAL(udpMcastFaces.size(), netifsV6.size());
  expectedAddr = boost::asio::ip::address_v6::from_string("ff02::1102");
  expectedAddr.scope_id(netifsV6.front()->getIndex());
  BOOST_CHECK_EQUAL(udpMcastFaces.front()->getRemoteUri(), FaceUri(udp::Endpoint(expectedAddr, 7012)));
}

BOOST_FIXTURE_TEST_CASE(Whitelist, UdpFactoryMcastFixture)
{
#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__
  SKIP_IF_UDP_MCAST_NETIF_COUNT_LT(1);

  std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        whitelist
        {
          ifname %ifname
        }
      }
    }
  )CONFIG";
  boost::replace_first(CONFIG, "%ifname", netifs.front()->getName());

  parseConfig(CONFIG, false);

  auto udpMcastFaces = this->listUdp4McastFaces();
  BOOST_CHECK_LE(udpMcastFaces.size(), 1);
  auto udpMcastFacesV6 = this->listUdp6McastFaces();
  BOOST_CHECK_LE(udpMcastFacesV6.size(), 1);
  udpMcastFaces.insert(udpMcastFaces.end(), udpMcastFacesV6.begin(), udpMcastFacesV6.end());
  BOOST_CHECK_GE(udpMcastFaces.size(), 1);
  BOOST_CHECK(std::all_of(udpMcastFaces.begin(), udpMcastFaces.end(),
                          [this] (const Face* face) { return isFaceOnNetif(*face, *netifs.front()); }));
}

BOOST_FIXTURE_TEST_CASE(Blacklist, UdpFactoryMcastFixture)
{
#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__
  SKIP_IF_UDP_MCAST_NETIF_COUNT_LT(1);

  std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        blacklist
        {
          ifname %ifname
        }
      }
    }
  )CONFIG";
  boost::replace_first(CONFIG, "%ifname", netifs.front()->getName());

  parseConfig(CONFIG, false);

  auto udpMcastFaces = this->listUdp4McastFaces();
  if (!netifsV4.empty())
    BOOST_CHECK_GE(udpMcastFaces.size(), netifsV4.size() - 1);
  auto udpMcastFacesV6 = this->listUdp6McastFaces();
  if (!netifsV6.empty())
    BOOST_CHECK_GE(udpMcastFacesV6.size(), netifsV6.size() - 1);
  udpMcastFaces.insert(udpMcastFaces.end(), udpMcastFacesV6.begin(), udpMcastFacesV6.end());
  BOOST_CHECK_LT(udpMcastFaces.size(), netifsV4.size() + netifsV6.size());
  BOOST_CHECK(std::none_of(udpMcastFaces.begin(), udpMcastFaces.end(),
                           [this] (const Face* face) { return isFaceOnNetif(*face, *netifs.front()); }));
}

BOOST_FIXTURE_TEST_CASE(ChangePredicate, UdpFactoryMcastFixture)
{
#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__
  SKIP_IF_UDP_MCAST_NETIF_COUNT_LT(2);

  std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        whitelist
        {
          ifname %ifname
        }
      }
    }
  )CONFIG";
  std::string CONFIG2 = CONFIG1;
  boost::replace_first(CONFIG1, "%ifname", netifs.front()->getName());
  boost::replace_first(CONFIG2, "%ifname", netifs.back()->getName());

  parseConfig(CONFIG1, false);

  auto udpMcastFaces = this->listUdp4McastFaces();
  auto udpMcastFacesV6 = this->listUdp6McastFaces();
  udpMcastFaces.insert(udpMcastFaces.end(), udpMcastFacesV6.begin(), udpMcastFacesV6.end());
  BOOST_CHECK_GE(udpMcastFaces.size(), 1);
  BOOST_CHECK(std::all_of(udpMcastFaces.begin(), udpMcastFaces.end(),
                          [this] (const Face* face) { return isFaceOnNetif(*face, *netifs.front()); }));

  parseConfig(CONFIG2, false);
  g_io.poll();

  udpMcastFaces = this->listUdp4McastFaces();
  udpMcastFacesV6 = this->listUdp6McastFaces();
  udpMcastFaces.insert(udpMcastFaces.end(), udpMcastFacesV6.begin(), udpMcastFacesV6.end());
  BOOST_CHECK_GE(udpMcastFaces.size(), 1);
  BOOST_CHECK(std::all_of(udpMcastFaces.begin(), udpMcastFaces.end(),
                          [this] (const Face* face) { return isFaceOnNetif(*face, *netifs.back()); }));
}

BOOST_AUTO_TEST_CASE(Omitted)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
  BOOST_CHECK_EQUAL(this->listFacesByScheme("udp4", ndn::nfd::LINK_TYPE_MULTI_ACCESS).size(), 0);
  BOOST_CHECK_EQUAL(this->listFacesByScheme("udp6", ndn::nfd::LINK_TYPE_MULTI_ACCESS).size(), 0);
}

BOOST_AUTO_TEST_CASE(AllDisabled)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        enable_v4 no
        enable_v6 no
        mcast no
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadListen)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        listen hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(BadPort, 2) // Bug #4489
BOOST_AUTO_TEST_CASE(BadPort)
{
  // not a number
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        port hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG1, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG1, false), ConfigFile::Error);

  // negative number
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      udp
      {
        port -1
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG2, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG2, false), ConfigFile::Error);

  // out of range
  const std::string CONFIG3 = R"CONFIG(
    face_system
    {
      udp
      {
        port 65536
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG3, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG3, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(BadIdleTimeout, 2) // Bug #4489
BOOST_AUTO_TEST_CASE(BadIdleTimeout)
{
  // not a number
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        idle_timeout hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG1, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG1, false), ConfigFile::Error);

  // negative number
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      udp
      {
        idle_timeout -15
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG2, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG2, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadMcast)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        mcast hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadMcastGroupV4)
{
  // not an address
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG1, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG1, false), ConfigFile::Error);

  // non-multicast address
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group 10.0.0.1
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG2, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG2, false), ConfigFile::Error);

  // wrong address family
  const std::string CONFIG3 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group ff02::1234
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG3, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG3, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadMcastGroupV6)
{
  // not an address
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group_v6 foo
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG1, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG1, false), ConfigFile::Error);

  // non-multicast address
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group_v6 fe80::1234
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG2, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG2, false), ConfigFile::Error);

  // wrong address family
  const std::string CONFIG3 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_group_v6 224.0.23.170
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG3, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG3, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadMcastPortV4)
{
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_port hey
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG1, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG1, false), ConfigFile::Error);

  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_port 99999
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG2, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG2, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadMcastPortV6)
{
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_port_v6 bar
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG1, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG1, false), ConfigFile::Error);

  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      udp
      {
        mcast_port_v6 99999
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG2, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG2, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(UnknownOption)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

BOOST_AUTO_TEST_CASE(GetChannels)
{
  BOOST_CHECK_EQUAL(factory.getChannels().empty(), true);

  std::set<std::string> expected;
  expected.insert(createChannel("127.0.0.1", 20070)->getUri().toString());
  expected.insert(createChannel("127.0.0.1", 20071)->getUri().toString());
  expected.insert(createChannel("::1", 20071)->getUri().toString());
  checkChannelListEqual(factory, expected);
}

BOOST_FIXTURE_TEST_CASE(CreateChannel, UdpFactoryMcastFixture)
{
  auto channel1 = createChannel("127.0.0.1", 20070);
  auto channel1a = createChannel("127.0.0.1", 20070);
  BOOST_CHECK_EQUAL(channel1, channel1a);
  BOOST_CHECK_EQUAL(channel1->getUri().toString(), "udp4://127.0.0.1:20070");

  auto channel2 = createChannel("127.0.0.1", 20071);
  BOOST_CHECK_NE(channel1, channel2);

  auto channel3 = createChannel("::1", 20071);
  BOOST_CHECK_NE(channel2, channel3);
  BOOST_CHECK_EQUAL(channel3->getUri().toString(), "udp6://[::1]:20071");

#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__

  // createChannel with a local endpoint that has already been allocated for a UDP multicast face
  if (!netifsV4.empty()) {
    auto mcastFace = createMulticastFace("127.0.0.1", "224.0.0.254", 20072);
    BOOST_CHECK_EXCEPTION(createChannel("127.0.0.1", 20072), UdpFactory::Error,
                          [] (const UdpFactory::Error& e) {
                            return strcmp(e.what(),
                                          "Cannot create UDP channel on 127.0.0.1:20072, "
                                          "endpoint already allocated for a UDP multicast face") == 0;
                          });
  }
  if (!netifsV6.empty()) {
    auto mcastFace = createMulticastFace("::1", "ff02::114", 20072);
    BOOST_CHECK_EXCEPTION(createChannel("::1", 20072), UdpFactory::Error,
                          [] (const UdpFactory::Error& e) {
                            return strcmp(e.what(),
                                          "Cannot create UDP channel on [::1]:20072, "
                                          "endpoint already allocated for a UDP multicast face") == 0;
                          });
  }
}

BOOST_FIXTURE_TEST_CASE(CreateMulticastFaceV4, UdpFactoryMcastFixture)
{
#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__
  SKIP_IF_UDP_MCAST_V4_NETIF_COUNT_LT(1);

  auto multicastFace1  = createMulticastFace("127.0.0.1", "224.0.0.254", 20070);
  auto multicastFace1a = createMulticastFace("127.0.0.1", "224.0.0.254", 20070);
  auto multicastFace2  = createMulticastFace("127.0.0.1", "224.0.0.254", 20030);
  BOOST_CHECK_EQUAL(multicastFace1, multicastFace1a);
  BOOST_CHECK_NE(multicastFace1, multicastFace2);

  auto address = findNonLoopbackAddressForMulticastFace(ndn::net::AddressFamily::V4);
  if (!address.is_unspecified()) {
    auto multicastFace3  = createMulticastFace(address.to_string(), "224.0.0.254", 20070);
    BOOST_CHECK_NE(multicastFace1, multicastFace3);
    BOOST_CHECK_NE(multicastFace2, multicastFace3);
  }

  // create with a local endpoint already used by a channel
  auto channel = createChannel("127.0.0.1", 20071);
  BOOST_CHECK_EXCEPTION(createMulticastFace("127.0.0.1", "224.0.0.254", 20071), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create UDP multicast face on 127.0.0.1:20071, "
                                        "endpoint already allocated for a UDP channel") == 0;
                        });

  // create with a local endpoint already used by a multicast face on a different multicast group
  BOOST_CHECK_EXCEPTION(createMulticastFace("127.0.0.1", "224.0.0.42", 20070), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create UDP multicast face on 127.0.0.1:20070, "
                                        "endpoint already allocated for a different UDP multicast face") == 0;
                        });
}

BOOST_FIXTURE_TEST_CASE(CreateMulticastFaceV6, UdpFactoryMcastFixture)
{
#ifdef __linux__
  // need superuser privileges to create multicast faces on Linux
  SKIP_IF_NOT_SUPERUSER();
#endif // __linux__
  SKIP_IF_UDP_MCAST_V6_NETIF_COUNT_LT(1);

  auto multicastFace1  = createMulticastFace("::1", "ff02::114", 20070);
  auto multicastFace1a = createMulticastFace("::1", "ff02::114", 20070);
  auto multicastFace2  = createMulticastFace("::1", "ff02::114", 20030);
  BOOST_CHECK_EQUAL(multicastFace1, multicastFace1a);
  BOOST_CHECK_NE(multicastFace1, multicastFace2);

  auto address = findNonLoopbackAddressForMulticastFace(ndn::net::AddressFamily::V6);
  if (!address.is_unspecified()) {
    auto multicastFace3  = createMulticastFace(address.to_string(), "ff02::114", 20070);
    BOOST_CHECK_NE(multicastFace1, multicastFace3);
    BOOST_CHECK_NE(multicastFace2, multicastFace3);
  }

  // create with a local endpoint already used by a channel
  auto channel = createChannel("::1", 20071);
  BOOST_CHECK_EXCEPTION(createMulticastFace("::1", "ff02::114", 20071), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create UDP multicast face on [::1]:20071, "
                                        "endpoint already allocated for a UDP channel") == 0;
                        });

  // create with a local endpoint already used by a multicast face on a different multicast group
  BOOST_CHECK_EXCEPTION(createMulticastFace("::1", "ff02::42", 20070), UdpFactory::Error,
                        [] (const UdpFactory::Error& e) {
                          return strcmp(e.what(),
                                        "Cannot create UDP multicast face on [::1]:20070, "
                                        "endpoint already allocated for a different UDP multicast face") == 0;
                        });
}

BOOST_AUTO_TEST_CASE(CreateFace)
{
  createFace(factory,
             FaceUri("udp4://127.0.0.1:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 504, "No channels available to connect"});

  createChannel("127.0.0.1", 20071);

  createFace(factory,
             FaceUri("udp4://127.0.0.1:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("udp4://127.0.0.1:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20072"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20073"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, false, true, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20073"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, false, false, true},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});
}

BOOST_AUTO_TEST_CASE(UnsupportedCreateFace)
{
  createChannel("127.0.0.1", 20071);

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20072"),
             FaceUri("udp4://127.0.0.1:20071"),
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Unicast UDP faces cannot be created with a LocalUri"});

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20072"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_ON_DEMAND, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Outgoing UDP faces do not support on-demand persistency"});

  createFace(factory,
             FaceUri("udp4://233.252.0.1:23252"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Cannot create multicast UDP faces"});

  createFace(factory,
             FaceUri("udp4://127.0.0.1:20072"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, true, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Local fields can only be enabled on faces with local scope"});
}

BOOST_AUTO_TEST_SUITE_END() // TestUdpFactory
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
