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

#include "face/ethernet-factory.hpp"
#include "face/face.hpp"

#include "factory-test-common.hpp"
#include "ethernet-fixture.hpp"
#include "face-system-fixture.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/algorithm/count_if.hpp>

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestEthernetFactory, EthernetFixture)

using face::Face;

class EthernetConfigFixture : public EthernetFixture
                            , public FaceSystemFixture
{
public:
  std::vector<const Face*>
  listEtherMcastFaces() const
  {
    return this->listFacesByScheme("ether", ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  }

  size_t
  countEtherMcastFaces() const
  {
    return this->listEtherMcastFaces().size();
  }
};

BOOST_FIXTURE_TEST_SUITE(ProcessConfig, EthernetConfigFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        mcast yes
        mcast_group 01:00:5E:00:17:AA
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

  BOOST_CHECK_EQUAL(this->countEtherMcastFaces(), netifs.size());
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

  BOOST_CHECK_EQUAL(this->countEtherMcastFaces(), 0);
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
  boost::replace_first(CONFIG, "%ifname", netifs.front().name);

  parseConfig(CONFIG, false);
  auto etherMcastFaces = this->listEtherMcastFaces();
  BOOST_REQUIRE_EQUAL(etherMcastFaces.size(), 1);
  BOOST_CHECK_EQUAL(etherMcastFaces.front()->getLocalUri().getHost(), netifs.front().name);
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
  boost::replace_first(CONFIG, "%ifname", netifs.front().name);

  parseConfig(CONFIG, false);
  auto etherMcastFaces = this->listEtherMcastFaces();
  BOOST_CHECK_EQUAL(etherMcastFaces.size(), netifs.size() - 1);
  BOOST_CHECK_EQUAL(boost::count_if(etherMcastFaces, [=] (const Face* face) {
    return face->getLocalUri().getHost() == netifs.front().name;
  }), 0);
}

BOOST_AUTO_TEST_CASE(EnableDisableMcast)
{
  const std::string CONFIG_WITH_MCAST = R"CONFIG(
    face_system
    {
      ether
      {
        mcast yes
      }
    }
  )CONFIG";
  const std::string CONFIG_WITHOUT_MCAST = R"CONFIG(
    face_system
    {
      ether
      {
        mcast no
      }
    }
  )CONFIG";

  parseConfig(CONFIG_WITHOUT_MCAST, false);
  BOOST_CHECK_EQUAL(this->countEtherMcastFaces(), 0);

  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  parseConfig(CONFIG_WITH_MCAST, false);
  g_io.poll();
  BOOST_CHECK_EQUAL(this->countEtherMcastFaces(), netifs.size());

  parseConfig(CONFIG_WITHOUT_MCAST, false);
  g_io.poll();
  BOOST_CHECK_EQUAL(this->countEtherMcastFaces(), 0);
}

BOOST_AUTO_TEST_CASE(ChangeMcastGroup)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      ether
      {
        mcast_group 01:00:00:00:00:01
      }
    }
  )CONFIG";
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      ether
      {
        mcast_group 01:00:00:00:00:02
      }
    }
  )CONFIG";

  parseConfig(CONFIG1, false);
  auto etherMcastFaces = this->listEtherMcastFaces();
  BOOST_REQUIRE_EQUAL(etherMcastFaces.size(), netifs.size());
  BOOST_CHECK_EQUAL(etherMcastFaces.front()->getRemoteUri(),
                    FaceUri(ethernet::Address(0x01, 0x00, 0x00, 0x00, 0x00, 0x01)));

  parseConfig(CONFIG2, false);
  g_io.poll();
  etherMcastFaces = this->listEtherMcastFaces();
  BOOST_REQUIRE_EQUAL(etherMcastFaces.size(), netifs.size());
  BOOST_CHECK_EQUAL(etherMcastFaces.front()->getRemoteUri(),
                    FaceUri(ethernet::Address(0x01, 0x00, 0x00, 0x00, 0x00, 0x02)));
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
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        mcast yes
        mcast_group hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(UnicastMcastGroup)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      ether
      {
        mcast yes
        mcast_group 02:00:00:00:00:01
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
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

BOOST_AUTO_TEST_CASE(GetChannels)
{
  EthernetFactory factory;

  auto channels = factory.getChannels();
  BOOST_CHECK_EQUAL(channels.empty(), true);
}

BOOST_AUTO_TEST_CASE(UnsupportedFaceCreate)
{
  EthernetFactory factory;

  createFace(factory,
             FaceUri("ether://[08:00:27:01:01:01]"),
             ndn::nfd::FACE_PERSISTENCY_PERMANENT,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});

  createFace(factory,
             FaceUri("ether://[08:00:27:01:01:01]"),
             ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});

  createFace(factory,
             FaceUri("ether://[08:00:27:01:01:01]"),
             ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});
}

BOOST_AUTO_TEST_SUITE_END() // TestEthernetFactory
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
