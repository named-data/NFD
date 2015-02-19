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

#include "rib/rib-status-publisher.hpp"
#include "rib-status-publisher-common.hpp"

#include "tests/test-common.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd {
namespace rib {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestRibStatusPublisher, RibStatusPublisherFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  Rib rib;

  Route route;
  Name name("/");
  route.faceId = 1;
  route.origin = 128;
  route.cost = 32;
  route.flags = ndn::nfd::ROUTE_FLAG_CAPTURE;
  rib.insert(name, route);

  ndn::KeyChain keyChain;
  shared_ptr<ndn::util::DummyClientFace> face = ndn::util::makeDummyClientFace();
  RibStatusPublisher publisher(rib, *face, "/localhost/nfd/rib/list", keyChain);

  publisher.publish();
  face->processEvents(time::milliseconds(1));

  BOOST_REQUIRE_EQUAL(face->sentDatas.size(), 1);
  decodeRibEntryBlock(face->sentDatas[0], name, route);
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace rib
} // namespace nfd
