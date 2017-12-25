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

#include "transport-test-common.hpp"

#include "ethernet-fixture.hpp"

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestMulticastEthernetTransport, EthernetFixture)

BOOST_AUTO_TEST_CASE(StaticProperties)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);
  initializeMulticast();

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(), FaceUri("dev://" + localEp));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(), FaceUri("ether://[" + remoteEp.toString() + "]"));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  BOOST_CHECK_EQUAL(transport->getSendQueueCapacity(), QUEUE_UNSUPPORTED);
}

BOOST_AUTO_TEST_CASE(PersistencyChange)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);
  initializeMulticast();

  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND), false);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERSISTENT), false);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERMANENT), true);
}

BOOST_AUTO_TEST_CASE(Close)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);
  initializeMulticast();

  transport->afterStateChange.connectSingleShot([] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::UP);
    BOOST_CHECK_EQUAL(newState, TransportState::CLOSING);
  });

  transport->close();

  transport->afterStateChange.connectSingleShot([this] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::CLOSING);
    BOOST_CHECK_EQUAL(newState, TransportState::CLOSED);
    limitedIo.afterOp();
  });

  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
}

BOOST_AUTO_TEST_CASE(SendQueueLength)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);
  initializeMulticast();

  BOOST_CHECK_EQUAL(transport->getSendQueueLength(), QUEUE_UNSUPPORTED);
}

BOOST_AUTO_TEST_SUITE_END() // TestMulticastEthernetTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
