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

#include "face/ethernet-transport.hpp"
#include "transport-test-common.hpp"

#include "network-interface-fixture.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestEthernetTransport, NetworkInterfaceFixture)

BOOST_AUTO_TEST_CASE(StaticProperties)
{
  SKIP_IF_NETWORK_INTERFACE_COUNT_LT(1);

  auto netif = m_interfaces.front();
  EthernetTransport transport(netif, ethernet::getDefaultMulticastAddress());
  checkStaticPropertiesInitialized(transport);

  BOOST_CHECK_EQUAL(transport.getLocalUri(), FaceUri::fromDev(netif.name));
  BOOST_CHECK_EQUAL(transport.getRemoteUri(), FaceUri(ethernet::getDefaultMulticastAddress()));
  BOOST_CHECK_EQUAL(transport.getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(transport.getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(transport.getLinkType(), ndn::nfd::LINK_TYPE_MULTI_ACCESS);
}

// TODO add the equivalent of these test cases from ethernet.t.cpp as of commit:65caf200924b28748037750449e28bcb548dbc9c
// SendPacket
// ProcessIncomingPacket

BOOST_AUTO_TEST_SUITE_END() // TestEthernetTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
