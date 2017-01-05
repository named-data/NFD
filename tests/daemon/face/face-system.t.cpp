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

#include "face/face-system.hpp"
#include "face-system-fixture.hpp"

// ProtocolFactory includes, sorted alphabetically
#ifdef HAVE_LIBPCAP
#include "face/ethernet-factory.hpp"
#endif // HAVE_LIBPCAP
#include "face/udp-factory.hpp"
#ifdef HAVE_UNIX_SOCKETS
#include "face/unix-stream-factory.hpp"
#endif // HAVE_UNIX_SOCKETS
#ifdef HAVE_WEBSOCKET
#include "face/websocket-factory.hpp"
#endif // HAVE_WEBSOCKET

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestFaceSystem, FaceSystemFixture)

BOOST_AUTO_TEST_SUITE(ProcessConfig)

class DummyProtocolFactory : public ProtocolFactory
{
public:
  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) override
  {
    processConfigHistory.push_back({configSection, context.isDryRun});
    if (!context.isDryRun) {
      this->providedSchemes = this->newProvidedSchemes;
    }
  }

  void
  createFace(const FaceUri& uri,
             ndn::nfd::FacePersistency persistency,
             bool wantLocalFieldsEnabled,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) override
  {
    BOOST_FAIL("createFace should not be called");
  }

  std::vector<shared_ptr<const Channel>>
  getChannels() const override
  {
    BOOST_FAIL("getChannels should not be called");
    return {};
  }

public:
  struct ProcessConfigArgs
  {
    OptionalConfigSection configSection;
    bool isDryRun;
  };
  std::vector<ProcessConfigArgs> processConfigHistory;

  std::set<std::string> newProvidedSchemes;
};

BOOST_AUTO_TEST_CASE(Normal)
{
  auto f1 = make_shared<DummyProtocolFactory>();
  auto f2 = make_shared<DummyProtocolFactory>();
  faceSystem.m_factories["f1"] = f1;
  faceSystem.m_factories["f2"] = f2;

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f1
      {
        key v1
      }
      f2
      {
        key v2
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, true);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().configSection->get<std::string>("key"), "v1");
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, true);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().configSection->get<std::string>("key"), "v2");

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, false);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().configSection->get<std::string>("key"), "v1");
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, false);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().configSection->get<std::string>("key"), "v2");
}

BOOST_AUTO_TEST_CASE(OmittedSection)
{
  auto f1 = make_shared<DummyProtocolFactory>();
  auto f2 = make_shared<DummyProtocolFactory>();
  faceSystem.m_factories["f1"] = f1;
  faceSystem.m_factories["f2"] = f2;

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f1
      {
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, true);
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, true);
  BOOST_CHECK(!f2->processConfigHistory.back().configSection);

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, false);
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, false);
  BOOST_CHECK(!f2->processConfigHistory.back().configSection);
}

BOOST_AUTO_TEST_CASE(UnknownSection)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f0
      {
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ChangeProvidedSchemes)
{
  auto f1 = make_shared<DummyProtocolFactory>();
  faceSystem.m_factories["f1"] = f1;

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f1
      {
      }
    }
  )CONFIG";

  f1->newProvidedSchemes.insert("s1");
  f1->newProvidedSchemes.insert("s2");
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
  BOOST_CHECK(faceSystem.getFactoryByScheme("f1") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s1"), f1.get());
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s2"), f1.get());

  f1->newProvidedSchemes.erase("s2");
  f1->newProvidedSchemes.insert("s3");
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
  BOOST_CHECK(faceSystem.getFactoryByScheme("f1") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s1"), f1.get());
  BOOST_CHECK(faceSystem.getFactoryByScheme("s2") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s3"), f1.get());
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

///\todo #3904 move Config* to *Factory test suite

BOOST_AUTO_TEST_SUITE(ConfigUdp)

BOOST_AUTO_TEST_CASE(Normal)
{
  SKIP_IF_NOT_SUPERUSER();

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        port 6363
        enable_v4 yes
        enable_v6 yes
        idle_timeout 30
        keep_alive_interval 25
        mcast yes
        mcast_port 56363
        mcast_group 224.0.23.170
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));

  auto& factory = this->getFactoryByScheme<UdpFactory>("udp");
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 2);
}

BOOST_AUTO_TEST_CASE(BadIdleTimeout)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        idle_timeout hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
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

BOOST_AUTO_TEST_CASE(BadMcastGroup)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        mcast no
        mcast_port 50
        mcast_group hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadMcastGroupV6)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        mcast no
        mcast_port 50
        mcast_group ::1
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ChannelsDisabled)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        port 6363
        enable_v4 no
        enable_v6 no
        idle_timeout 30
        keep_alive_interval 25
        mcast yes
        mcast_port 56363
        mcast_group 224.0.23.170
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ConflictingMcast)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      udp
      {
        port 6363
        enable_v4 no
        enable_v6 yes
        idle_timeout 30
        keep_alive_interval 25
        mcast yes
        mcast_port 56363
        mcast_group 224.0.23.170
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
      udp
      {
        hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(MulticastReinit)
{
  SKIP_IF_NOT_SUPERUSER();

  const std::string CONFIG_WITH_MCAST = R"CONFIG(
    face_system
    {
      udp
      {
        mcast yes
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITH_MCAST, false));

  auto& factory = this->getFactoryByScheme<UdpFactory>("udp");

  if (factory.getMulticastFaces().empty()) {
    BOOST_WARN_MESSAGE(false, "skipping assertions that require at least one UDP multicast face");
    return;
  }

  const std::string CONFIG_WITHOUT_MCAST = R"CONFIG(
    face_system
    {
      udp
      {
        mcast no
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITHOUT_MCAST, false));
  BOOST_REQUIRE_NO_THROW(g_io.poll());
  BOOST_CHECK_EQUAL(factory.getMulticastFaces().size(), 0);
}
BOOST_AUTO_TEST_SUITE_END() // ConfigUdp

#ifdef HAVE_LIBPCAP
BOOST_AUTO_TEST_SUITE(ConfigEther)

BOOST_AUTO_TEST_CASE(Normal)
{
  SKIP_IF_NOT_SUPERUSER();

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

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));

  auto& factory = this->getFactoryByScheme<EthernetFactory>("ether");
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
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
        mcast_group
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

BOOST_AUTO_TEST_CASE(MulticastReinit)
{
  SKIP_IF_NOT_SUPERUSER();

  const std::string CONFIG_WITH_MCAST = R"CONFIG(
    face_system
    {
      ether
      {
        mcast yes
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITH_MCAST, false));

  auto& factory = this->getFactoryByScheme<EthernetFactory>("ether");

  if (factory.getMulticastFaces().empty()) {
    BOOST_WARN_MESSAGE(false, "skipping assertions that require at least one Ethernet multicast face");
    return;
  }

  const std::string CONFIG_WITHOUT_MCAST = R"CONFIG(
    face_system
    {
      ether
      {
        mcast no
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITHOUT_MCAST, false));
  BOOST_REQUIRE_NO_THROW(g_io.poll());
  BOOST_CHECK_EQUAL(factory.getMulticastFaces().size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // ConfigEther
#endif // HAVE_LIBPCAP

#ifdef HAVE_WEBSOCKET
BOOST_AUTO_TEST_SUITE(ConfigWebSocket)

BOOST_AUTO_TEST_CASE(Normal)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      websocket
      {
        listen yes
        port 9696
        enable_v4 yes
        enable_v6 yes
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));

  auto& factory = this->getFactoryByScheme<WebSocketFactory>("websocket");
  BOOST_CHECK_EQUAL(factory.getChannels().size(), 1);
}

BOOST_AUTO_TEST_CASE(ChannelsDisabled)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      websocket
      {
        listen yes
        port 9696
        enable_v4 no
        enable_v6 no
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(Ipv4ChannelDisabled)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      websocket
      {
        listen yes
        port 9696
        enable_v4 no
        enable_v6 yes
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // ConfigWebSocket
#endif // HAVE_WEBSOCKET

BOOST_AUTO_TEST_SUITE_END() // TestFaceSystem
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace face
} // namespace nfd
