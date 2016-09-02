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

#include "mgmt/face-manager.hpp"
#include "face/udp-factory.hpp"

#ifdef HAVE_LIBPCAP
#include "face/ethernet-factory.hpp"
#endif // HAVE_LIBPCAP

#include "nfd-manager-common-fixture.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_AUTO_TEST_SUITE(TestFaceManager)

class FaceManagerProcessConfigFixture : public NfdManagerCommonFixture
{
public:
  FaceManagerProcessConfigFixture()
    : m_manager(m_forwarder.getFaceTable(), m_dispatcher, *m_authenticator)
  {
    m_manager.setConfigFile(m_config);
  }

  void
  parseConfig(const std::string& type, bool isDryRun)
  {
    m_config.parse(type, isDryRun, "test-config");
  }

protected:
  FaceManager m_manager;
  ConfigFile m_config;
};

BOOST_FIXTURE_TEST_SUITE(ProcessConfig, FaceManagerProcessConfigFixture)

#ifdef HAVE_UNIX_SOCKETS

BOOST_AUTO_TEST_CASE(ProcessSectionUnix)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  unix\n"
    "  {\n"
    "    path /tmp/nfd.sock\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
}

BOOST_AUTO_TEST_CASE(ProcessSectionUnixUnknownOption)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  unix\n"
    "  {\n"
    "    hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

#endif // HAVE_UNIX_SOCKETS

BOOST_AUTO_TEST_CASE(ProcessSectionTcp)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    listen yes\n"
    "    port 16363\n"
    "    enable_v4 yes\n"
    "    enable_v6 yes\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
}

BOOST_AUTO_TEST_CASE(ProcessSectionTcpBadListen)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    listen hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionTcpChannelsDisabled)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    port 6363\n"
    "    enable_v4 no\n"
    "    enable_v6 no\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionTcpUnknownOption)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdp)
{
  SKIP_IF_NOT_SUPERUSER();

  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    port 6363\n"
    "    enable_v4 yes\n"
    "    enable_v6 yes\n"
    "    idle_timeout 30\n"
    "    keep_alive_interval 25\n"
    "    mcast yes\n"
    "    mcast_port 56363\n"
    "    mcast_group 224.0.23.170\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdpBadIdleTimeout)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    idle_timeout hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdpBadMcast)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdpBadMcastGroup)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast no\n"
    "    mcast_port 50\n"
    "    mcast_group hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdpBadMcastGroupV6)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast no\n"
    "    mcast_port 50\n"
    "    mcast_group ::1\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdpChannelsDisabled)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    port 6363\n"
    "    enable_v4 no\n"
    "    enable_v6 no\n"
    "    idle_timeout 30\n"
    "    keep_alive_interval 25\n"
    "    mcast yes\n"
    "    mcast_port 56363\n"
    "    mcast_group 224.0.23.170\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdpConflictingMcast)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    port 6363\n"
    "    enable_v4 no\n"
    "    enable_v6 yes\n"
    "    idle_timeout 30\n"
    "    keep_alive_interval 25\n"
    "    mcast yes\n"
    "    mcast_port 56363\n"
    "    mcast_group 224.0.23.170\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdpUnknownOption)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionUdpMulticastReinit)
{
  SKIP_IF_NOT_SUPERUSER();

  const std::string CONFIG_WITH_MCAST =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast yes\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITH_MCAST, false));

  BOOST_REQUIRE(m_manager.m_factories.find("udp") != m_manager.m_factories.end());
  auto factory = dynamic_pointer_cast<UdpFactory>(m_manager.m_factories.find("udp")->second);
  BOOST_REQUIRE(factory != nullptr);

  if (factory->getMulticastFaces().empty()) {
    BOOST_WARN_MESSAGE(false, "skipping assertions that require at least one UDP multicast face");
    return;
  }

  const std::string CONFIG_WITHOUT_MCAST =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast no\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITHOUT_MCAST, false));
  BOOST_REQUIRE_NO_THROW(g_io.poll());
  BOOST_CHECK_EQUAL(factory->getMulticastFaces().size(), 0);
}

#ifdef HAVE_LIBPCAP

BOOST_AUTO_TEST_CASE(ProcessSectionEther)
{
  SKIP_IF_NOT_SUPERUSER();

  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast yes\n"
    "    mcast_group 01:00:5E:00:17:AA\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
}

BOOST_AUTO_TEST_CASE(ProcessSectionEtherBadMcast)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionEtherBadMcastGroup)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast yes\n"
    "    mcast_group\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionEtherUnknownOption)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ProcessSectionEtherMulticastReinit)
{
  SKIP_IF_NOT_SUPERUSER();

  const std::string CONFIG_WITH_MCAST =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast yes\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITH_MCAST, false));

  BOOST_REQUIRE(m_manager.m_factories.find("ether") != m_manager.m_factories.end());
  auto factory = dynamic_pointer_cast<EthernetFactory>(m_manager.m_factories.find("ether")->second);
  BOOST_REQUIRE(factory != nullptr);

  if (factory->getMulticastFaces().empty()) {
    BOOST_WARN_MESSAGE(false, "skipping assertions that require at least one Ethernet multicast face");
    return;
  }

  const std::string CONFIG_WITHOUT_MCAST =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast no\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITHOUT_MCAST, false));
  BOOST_REQUIRE_NO_THROW(g_io.poll());
  BOOST_CHECK_EQUAL(factory->getMulticastFaces().size(), 0);
}

#endif // HAVE_LIBPCAP

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig
BOOST_AUTO_TEST_SUITE_END() // TestFaceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
