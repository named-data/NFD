/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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
#include "fw/face-table.hpp"

// ProtocolFactory includes, sorted alphabetically
#ifdef HAVE_LIBPCAP
#include "face/ethernet-factory.hpp"
#endif // HAVE_LIBPCAP
#include "face/tcp-factory.hpp"
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

class FaceSystemFixture : public BaseFixture
{
public:
  FaceSystemFixture()
    : faceSystem(faceTable)
  {
    faceSystem.setConfigFile(configFile);
  }

  void
  parseConfig(const std::string& text, bool isDryRun)
  {
    configFile.parse(text, isDryRun, "test-config");
  }

protected:
  FaceTable faceTable;
  FaceSystem faceSystem;
  ConfigFile configFile;
};

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestFaceSystem, FaceSystemFixture)

#ifdef HAVE_UNIX_SOCKETS
BOOST_AUTO_TEST_SUITE(ConfigUnix)

BOOST_AUTO_TEST_CASE(Normal)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      unix
      {
        path /tmp/nfd.sock
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));

  auto factory = dynamic_cast<UnixStreamFactory*>(faceSystem.getProtocolFactory("unix"));
  BOOST_REQUIRE(factory != nullptr);
  BOOST_CHECK_EQUAL(factory->getChannels().size(), 1);
}

BOOST_AUTO_TEST_CASE(UnknownOption)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      unix
      {
        hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // ConfigUnix
#endif // HAVE_UNIX_SOCKETS

BOOST_AUTO_TEST_SUITE(ConfigTcp)

BOOST_AUTO_TEST_CASE(Normal)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        listen yes
        port 16363
        enable_v4 yes
        enable_v6 yes
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));

  auto factory = dynamic_cast<TcpFactory*>(faceSystem.getProtocolFactory("tcp"));
  BOOST_REQUIRE(factory != nullptr);
  BOOST_CHECK_EQUAL(factory->getChannels().size(), 2);
}

BOOST_AUTO_TEST_CASE(BadListen)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        listen hello
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
      tcp
      {
        port 6363
        enable_v4 no
        enable_v6 no
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
      tcp
      {
        hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // ConfigTcp

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

  auto factory = dynamic_cast<UdpFactory*>(faceSystem.getProtocolFactory("udp"));
  BOOST_REQUIRE(factory != nullptr);
  BOOST_CHECK_EQUAL(factory->getChannels().size(), 2);
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

  auto factory = dynamic_cast<UdpFactory*>(faceSystem.getProtocolFactory("udp"));
  BOOST_REQUIRE(factory != nullptr);

  if (factory->getMulticastFaces().empty()) {
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
  BOOST_CHECK_EQUAL(factory->getMulticastFaces().size(), 0);
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

  auto factory = dynamic_cast<EthernetFactory*>(faceSystem.getProtocolFactory("ether"));
  BOOST_REQUIRE(factory != nullptr);
  BOOST_CHECK_EQUAL(factory->getChannels().size(), 0);
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

  auto factory = dynamic_cast<EthernetFactory*>(faceSystem.getProtocolFactory("ether"));
  BOOST_REQUIRE(factory != nullptr);

  if (factory->getMulticastFaces().empty()) {
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
  BOOST_CHECK_EQUAL(factory->getMulticastFaces().size(), 0);
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

  auto factory = dynamic_cast<WebSocketFactory*>(faceSystem.getProtocolFactory("websocket"));
  BOOST_REQUIRE(factory != nullptr);
  BOOST_CHECK_EQUAL(factory->getChannels().size(), 1);
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
