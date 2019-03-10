/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "face/null-face.hpp"
#include "face/null-transport.hpp"

#include "tests/daemon/global-io-fixture.hpp"
#include "transport-test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestNullFace, GlobalIoFixture)

BOOST_AUTO_TEST_CASE(StaticProperties)
{
  FaceUri uri("testnull://hhppt12sy");
  auto face = makeNullFace(uri);
  auto transport = face->getTransport();

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(), uri);
  BOOST_CHECK_EQUAL(transport->getRemoteUri(), uri);
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(ExpirationTime)
{
  auto transport = make_unique<NullTransport>();

  BOOST_CHECK_EQUAL(transport->getExpirationTime(), time::steady_clock::TimePoint::max());
}

BOOST_AUTO_TEST_CASE(SendQueue)
{
  auto transport = make_unique<NullTransport>();

  BOOST_CHECK_EQUAL(transport->getSendQueueCapacity(), QUEUE_UNSUPPORTED);
  BOOST_CHECK_EQUAL(transport->getSendQueueLength(), QUEUE_UNSUPPORTED);
}

BOOST_AUTO_TEST_CASE(Send)
{
  auto face = makeNullFace();
  BOOST_CHECK_EQUAL(face->getState(), FaceState::UP);

  face->sendInterest(*makeInterest("/A"), 0);
  BOOST_CHECK_EQUAL(face->getState(), FaceState::UP);

  face->sendData(*makeData("/B"), 0);
  BOOST_CHECK_EQUAL(face->getState(), FaceState::UP);
}

BOOST_AUTO_TEST_CASE(PersistencyChange)
{
  auto transport = make_unique<NullTransport>();

  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND), false);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERSISTENT), false);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERMANENT), true);
}

BOOST_AUTO_TEST_CASE(Close)
{
  auto transport = make_unique<NullTransport>();
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);

  transport->close();
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::CLOSED);
}

BOOST_AUTO_TEST_SUITE_END() // TestNullFace
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
