/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "face/multicast-udp-transport.hpp"
#include "transport-properties.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;
namespace ip = boost::asio::ip;
using ip::udp;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestMulticastUdpTransport, BaseFixture)

BOOST_AUTO_TEST_CASE(StaticPropertiesIpv4)
{
  udp::socket sockRx(g_io);
  sockRx.open(udp::v4());
  sockRx.set_option(udp::socket::reuse_address(true));
  sockRx.bind(udp::endpoint(ip::address::from_string("127.0.0.1"), 7001));

  udp::socket sockTx(g_io);
  sockTx.open(udp::v4());
  sockTx.set_option(udp::socket::reuse_address(true));
  sockTx.bind(udp::endpoint(udp::v4(), 7001));

  MulticastUdpTransport transport(sockRx.local_endpoint(),
                                  udp::endpoint(ip::address::from_string("230.15.19.47"), 7001),
                                  std::move(sockRx), std::move(sockTx));
  checkStaticPropertiesInitialized(transport);

  BOOST_CHECK_EQUAL(transport.getLocalUri(), FaceUri("udp4://127.0.0.1:7001"));
  BOOST_CHECK_EQUAL(transport.getRemoteUri(), FaceUri("udp4://230.15.19.47:7001"));
  BOOST_CHECK_EQUAL(transport.getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(transport.getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(transport.getLinkType(), ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  BOOST_CHECK_EQUAL(transport.getMtu(), 65535 - 60 - 8);
}

BOOST_AUTO_TEST_SUITE_END() // TestMulticastUdpTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
