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

#include "face/ethernet-factory.hpp"

#include "factory-test-common.hpp"
#include "network-interface-fixture.hpp"

namespace nfd {
namespace tests {

using nfd::face::tests::NetworkInterfaceFixture;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestEthernetFactory, NetworkInterfaceFixture)

BOOST_AUTO_TEST_CASE(GetChannels)
{
  EthernetFactory factory;

  auto channels = factory.getChannels();
  BOOST_CHECK_EQUAL(channels.empty(), true);
}

BOOST_AUTO_TEST_CASE(MulticastFacesMap)
{
  SKIP_IF_NETWORK_INTERFACE_COUNT_LT(1);

  EthernetFactory factory;
  auto face1 = factory.createMulticastFace(m_interfaces.front(), ethernet::getBroadcastAddress());
  auto face1bis = factory.createMulticastFace(m_interfaces.front(), ethernet::getBroadcastAddress());
  BOOST_CHECK_EQUAL(face1, face1bis);

  auto face2 = factory.createMulticastFace(m_interfaces.front(), ethernet::getDefaultMulticastAddress());
  BOOST_CHECK_NE(face1, face2);

  SKIP_IF_NETWORK_INTERFACE_COUNT_LT(2);

  auto face3 = factory.createMulticastFace(m_interfaces.back(), ethernet::getBroadcastAddress());
  BOOST_CHECK_NE(face1, face3);
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
} // namespace nfd
