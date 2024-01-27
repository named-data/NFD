/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "face/ethernet-factory.hpp"

#include "ethernet-fixture.hpp"
#include "face-system-fixture.hpp"
#include "factory-test-common.hpp"

#include <boost/algorithm/string/replace.hpp>

namespace nfd::tests {

using face::EthernetFactory;

class EthernetFactoryFixture : public EthernetFixture
                             , public FaceSystemFactoryFixture<EthernetFactory>
{
protected:
  std::set<std::string>
  listUrisOfAvailableNetifs() const
  {
    std::set<std::string> uris;
    std::transform(netifs.begin(), netifs.end(), std::inserter(uris, uris.end()),
                   [] (const auto& netif) { return FaceUri::fromDev(netif->getName()).toString(); });
    return uris;
  }

  std::vector<const Face*>
  listEtherMcastFaces(ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_MULTI_ACCESS) const
  {
    return listFacesByScheme("ether", linkType);
  }

  shared_ptr<ndn::net::NetworkInterface>
  makeFakeNetif()
  {
    static int counter = 0;
    ++counter;

    auto netif = netmon->makeNetworkInterface();
    netif->setIndex(1000 + counter);
    netif->setName("ethdummy" + std::to_string(counter));
    netif->setType(ndn::net::InterfaceType::ETHERNET);
    netif->setFlags(IFF_MULTICAST | IFF_UP);
    netif->setState(ndn::net::InterfaceState::RUNNING);
    netif->setMtu(1500);
    netif->setEthernetAddress(ethernet::Address{0x00, 0x00, 0x5e, 0x00, 0x53, 0x5e});
    return netif;
  }
};

class EthernetFactoryFixtureWithRealNetifs : public EthernetFactoryFixture
{
protected:
  EthernetFactoryFixtureWithRealNetifs()
  {
    copyRealNetifsToNetmon();
  }
};

BOOST_AUTO_TEST_SUITE(Face)
BOOST_AUTO_TEST_SUITE(TestEthernetFactory)

BOOST_FIXTURE_TEST_SUITE(ProcessConfig, EthernetFactoryFixtureWithRealNetifs)

BOOST_AUTO_TEST_CASE(Defaults)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, this->listUrisOfAvailableNetifs());
  auto channels = factory.getChannels();
  BOOST_CHECK(std::all_of(channels.begin(), channels.end(),
                          [] (const auto& ch) { return ch->isListening(); }));
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), netifs.size());
}

BOOST_AUTO_TEST_CASE(DisableListenAndMcast)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        listen no
        idle_timeout 60
        mcast no
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, this->listUrisOfAvailableNetifs());
  auto channels = factory.getChannels();
  BOOST_CHECK(std::none_of(channels.begin(), channels.end(),
                           [] (const auto& ch) { return ch->isListening(); }));
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);
}

BOOST_AUTO_TEST_CASE(McastNormal)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        listen no
        mcast yes
        mcast_group 01:00:5E:00:17:AA
        mcast_ad_hoc no
        whitelist
        {
          *
        }
        blacklist
        {
        }
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  auto etherMcastFaces = this->listEtherMcastFaces();
  BOOST_CHECK_EQUAL(etherMcastFaces.size(), netifs.size());
  for (const auto* face : etherMcastFaces) {
    BOOST_REQUIRE(face->getChannel().lock());
    // not universal, but for Ethernet, local URI of a mcast face matches URI of the associated channel
    BOOST_CHECK_EQUAL(face->getLocalUri(), face->getChannel().lock()->getUri());
  }
}

BOOST_AUTO_TEST_CASE(EnableDisableMcast)
{
  const std::string CONFIG_WITH_MCAST = R"CONFIG(
    face_system
    {
      ether
      {
        listen no
        mcast yes
      }
    }
  )CONFIG";
  const std::string CONFIG_WITHOUT_MCAST = R"CONFIG(
    face_system
    {
      ether
      {
        listen no
        mcast no
      }
    }
  )CONFIG";

  parseConfig(CONFIG_WITHOUT_MCAST, false);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);

  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  parseConfig(CONFIG_WITH_MCAST, false);
  g_io.poll();
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), netifs.size());

  parseConfig(CONFIG_WITHOUT_MCAST, false);
  g_io.poll();
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);
}

BOOST_AUTO_TEST_CASE(McastAdHoc)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        listen no
        mcast_ad_hoc yes
      }
    }
  )CONFIG";

  parseConfig(CONFIG, false);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces(ndn::nfd::LINK_TYPE_MULTI_ACCESS).size(), 0);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces(ndn::nfd::LINK_TYPE_AD_HOC).size(), netifs.size());
}

BOOST_AUTO_TEST_CASE(ChangeMcastGroup)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      ether
      {
        mcast_group 01:00:5e:90:10:01
      }
    }
  )CONFIG";
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      ether
      {
        mcast_group 01:00:5e:90:10:02
      }
    }
  )CONFIG";

  parseConfig(CONFIG1, false);
  g_io.poll();
  auto etherMcastFaces = this->listEtherMcastFaces();
  BOOST_REQUIRE_EQUAL(etherMcastFaces.size(), netifs.size());
  BOOST_CHECK_EQUAL(etherMcastFaces.front()->getRemoteUri(),
                    FaceUri(ethernet::Address{0x01, 0x00, 0x5e, 0x90, 0x10, 0x01}));

  parseConfig(CONFIG2, false);
  g_io.poll();
  etherMcastFaces = this->listEtherMcastFaces();
  BOOST_REQUIRE_EQUAL(etherMcastFaces.size(), netifs.size());
  BOOST_CHECK_EQUAL(etherMcastFaces.front()->getRemoteUri(),
                    FaceUri(ethernet::Address{0x01, 0x00, 0x5e, 0x90, 0x10, 0x02}));
}

BOOST_AUTO_TEST_CASE(Whitelist)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        whitelist
        {
          ifname %ifname
        }
      }
    }
  )CONFIG";
  auto ifname = netifs.front()->getName();
  boost::replace_first(CONFIG, "%ifname", ifname);

  parseConfig(CONFIG, false);

  auto etherMcastFaces = this->listEtherMcastFaces();
  BOOST_REQUIRE_EQUAL(etherMcastFaces.size(), 1);
  BOOST_CHECK_EQUAL(etherMcastFaces.front()->getLocalUri(), FaceUri::fromDev(ifname));
}

BOOST_AUTO_TEST_CASE(Blacklist)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        blacklist
        {
          ifname %ifname
        }
      }
    }
  )CONFIG";
  auto ifname = netifs.front()->getName();
  boost::replace_first(CONFIG, "%ifname", ifname);

  parseConfig(CONFIG, false);

  auto etherMcastFaces = this->listEtherMcastFaces();
  BOOST_CHECK_EQUAL(etherMcastFaces.size(), netifs.size() - 1);
  BOOST_CHECK(std::none_of(etherMcastFaces.begin(), etherMcastFaces.end(),
                           [uri = FaceUri::fromDev(ifname)] (const auto* face) {
                             return face->getLocalUri() == uri;
                           }));
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

  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);
}

BOOST_AUTO_TEST_CASE(BadListen)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        listen hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadIdleTimeout)
{
  // not a number
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      ether
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
      ether
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
      ether
      {
        mcast hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadMcastGroup)
{
  // not an address
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      ether
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
      ether
      {
        mcast_group 00:00:5e:00:53:5e
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
      ether
      {
        hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

BOOST_FIXTURE_TEST_CASE(GetChannels, EthernetFactoryFixtureWithRealNetifs)
{
  BOOST_CHECK_EQUAL(factory.getChannels().empty(), true);

  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  factory.createChannel(netifs.front(), 1_min);
  checkChannelListEqual(factory, {FaceUri::fromDev(netifs.front()->getName()).toString()});
}

BOOST_FIXTURE_TEST_CASE(CreateChannel, EthernetFactoryFixtureWithRealNetifs)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  auto channel1 = factory.createChannel(netifs.front(), 1_min);
  auto channel1a = factory.createChannel(netifs.front(), 5_min);
  BOOST_CHECK_EQUAL(channel1, channel1a);
  checkChannelListEqual(factory, {FaceUri::fromDev(netifs.front()->getName()).toString()});

  SKIP_IF_ETHERNET_NETIF_COUNT_LT(2);

  auto channel2 = factory.createChannel(netifs.back(), 1_min);
  BOOST_CHECK_NE(channel1, channel2);
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 2);
}

BOOST_FIXTURE_TEST_CASE(CreateFace, EthernetFactoryFixtureWithRealNetifs)
{
  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5e]"),
             FaceUri("dev://eth0"),
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 504, "No channels available to connect"});

  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);
  auto localUri = factory.createChannel(netifs.front(), 1_min)->getUri();

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5e]"),
             localUri,
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5e]"),
             localUri,
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:53]"),
             localUri,
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:57]"),
             localUri,
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, {}, false, true, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5b]"),
             localUri,
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, {}, false, false, true},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5c]"),
             localUri,
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, 1000, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});
}

BOOST_FIXTURE_TEST_CASE(CreateFaceInvalidRequest, EthernetFactoryFixture)
{
  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5e]"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Creation of unicast Ethernet faces requires a LocalUri with dev:// scheme"});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5e]"),
             FaceUri("udp4://127.0.0.1:20071"),
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Creation of unicast Ethernet faces requires a LocalUri with dev:// scheme"});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5e]"),
             FaceUri("dev://eth0"),
             {ndn::nfd::FACE_PERSISTENCY_ON_DEMAND, {}, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Outgoing Ethernet faces do not support on-demand persistency"});

  createFace(factory,
             FaceUri("ether://[01:00:5e:90:10:5e]"),
             FaceUri("dev://eth0"),
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Cannot create multicast Ethernet faces"});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5e]"),
             FaceUri("dev://eth0"),
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, {}, true, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Local fields can only be enabled on faces with local scope"});

  createFace(factory,
             FaceUri("ether://[00:00:5e:00:53:5e]"),
             FaceUri("dev://eth0"),
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, 42, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Override MTU cannot be less than 64"});
}

BOOST_FIXTURE_TEST_SUITE(OnInterfaceAdded, EthernetFactoryFixture)

BOOST_AUTO_TEST_CASE(EligibleForChannelAndMcast)
{
  parseConfig(R"CONFIG(
    face_system
    {
      ether
    }
  )CONFIG", false);
  g_io.poll();
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);

  // Add a fake interface that satisfies both unicast and multicast criteria.
  netmon->addInterface(this->makeFakeNetif());
  // The channel fails to listen because the interface is fake.
  // However, it's not removed from the channel list (issue #4400).
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 1);
  // Multicast face creation fails because the interface is fake.
  // This test is to ensure that the factory handles failures gracefully.
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);

  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  // Now add a real interface: both channel and multicast face should be created successfully.
  netmon->addInterface(std::const_pointer_cast<ndn::net::NetworkInterface>(netifs.front()));
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 2);
  auto etherMcastFaces = this->listEtherMcastFaces();
  BOOST_REQUIRE_EQUAL(etherMcastFaces.size(), 1);
  BOOST_CHECK_EQUAL(etherMcastFaces.front()->getLocalUri(), FaceUri::fromDev(netifs.front()->getName()));
}

BOOST_AUTO_TEST_CASE(EligibleForChannelOnly)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  parseConfig(R"CONFIG(
    face_system
    {
      ether
      {
        listen no
      }
    }
  )CONFIG", false);
  g_io.poll();
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);

  // Add an interface that satisfies only the unicast criteria.
  auto netif = std::const_pointer_cast<ndn::net::NetworkInterface>(netifs.front());
  netif->setFlags(netif->getFlags() & ~IFF_MULTICAST);
  netmon->addInterface(netif);
  checkChannelListEqual(factory, {FaceUri::fromDev(netifs.front()->getName()).toString()});
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);
}

BOOST_AUTO_TEST_CASE(Ineligible)
{
  parseConfig(R"CONFIG(
    face_system
    {
      ether
    }
  )CONFIG", false);
  g_io.poll();

  // netif is down
  auto netif = this->makeFakeNetif();
  netif->setFlags(netif->getFlags() & ~IFF_UP);
  netmon->addInterface(netif);
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);

  // incompatible netif type
  netif = this->makeFakeNetif();
  netif->setType(ndn::net::InterfaceType::LOOPBACK);
  netmon->addInterface(netif);
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);

  // invalid Ethernet address
  netif = this->makeFakeNetif();
  netif->setEthernetAddress(ethernet::Address{});
  netmon->addInterface(netif);
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);
}

BOOST_AUTO_TEST_CASE(Disabled)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  parseConfig("", false);
  g_io.poll();

  netmon->addInterface(std::const_pointer_cast<ndn::net::NetworkInterface>(netifs.front()));
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
  BOOST_CHECK_EQUAL(this->listEtherMcastFaces().size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // OnInterfaceAdded

BOOST_AUTO_TEST_SUITE_END() // TestEthernetFactory
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace nfd::tests
