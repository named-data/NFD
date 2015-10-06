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

#include "face/tcp-transport.hpp"
#include "transport-properties.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;
namespace ip = boost::asio::ip;
using ip::tcp;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestTcpTransport, BaseFixture)

BOOST_AUTO_TEST_CASE(StaticPropertiesIpv4)
{
  tcp::endpoint ep(ip::address_v4::loopback(), 7002);
  tcp::acceptor acceptor(g_io, ep);

  tcp::socket sock(g_io, tcp::endpoint(ip::address_v4::loopback(), 7001));
  sock.connect(ep);
  TcpTransport transport(std::move(sock), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  checkStaticPropertiesInitialized(transport);

  BOOST_CHECK_EQUAL(transport.getLocalUri(), FaceUri("tcp4://127.0.0.1:7001"));
  BOOST_CHECK_EQUAL(transport.getRemoteUri(), FaceUri("tcp4://127.0.0.1:7002"));
  BOOST_CHECK_EQUAL(transport.getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
  BOOST_CHECK_EQUAL(transport.getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(transport.getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport.getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(StaticPropertiesIpv6)
{
  tcp::endpoint ep(ip::address_v6::loopback(), 7002);
  tcp::acceptor acceptor(g_io, ep);

  tcp::socket sock(g_io, tcp::endpoint(ip::address_v6::loopback(), 7001));
  sock.connect(ep);
  TcpTransport transport(std::move(sock), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  checkStaticPropertiesInitialized(transport);

  BOOST_CHECK_EQUAL(transport.getLocalUri(), FaceUri("tcp6://[::1]:7001"));
  BOOST_CHECK_EQUAL(transport.getRemoteUri(), FaceUri("tcp6://[::1]:7002"));
  BOOST_CHECK_EQUAL(transport.getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
  BOOST_CHECK_EQUAL(transport.getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(transport.getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport.getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_SUITE_END() // TestTcpTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
