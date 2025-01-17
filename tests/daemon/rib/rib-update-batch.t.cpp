/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#include "rib/rib-update-batch.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/rib/create-route.hpp"

namespace nfd::tests {

using namespace nfd::rib;

BOOST_AUTO_TEST_SUITE(Rib)
BOOST_AUTO_TEST_SUITE(TestRibUpdateBatch)

BOOST_AUTO_TEST_CASE(AddAndIterate)
{
  const uint64_t faceId = 1;
  RibUpdateBatch batch(faceId);

  Route routeRegister = createRoute(faceId, 128, 10, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  RibUpdate registerUpdate{RibUpdate::REGISTER, "/a", routeRegister};
  batch.add(registerUpdate);

  BOOST_CHECK_EQUAL(batch.getFaceId(), faceId);

  Route routeUnregister = createRoute(faceId, 0, 0, ndn::nfd::ROUTE_FLAG_CAPTURE);
  RibUpdate unregisterUpdate{RibUpdate::UNREGISTER, "/a/b", routeUnregister};
  batch.add(unregisterUpdate);

  BOOST_REQUIRE_EQUAL(batch.size(), 2);
  auto it = batch.begin();

  BOOST_CHECK_EQUAL(it->action, RibUpdate::REGISTER);
  BOOST_CHECK_EQUAL(it->name, "/a");
  BOOST_CHECK_EQUAL(it->route, routeRegister);

  ++it;
  BOOST_CHECK_EQUAL(it->action, RibUpdate::UNREGISTER);
  BOOST_CHECK_EQUAL(it->name, "/a/b");
  BOOST_CHECK_EQUAL(it->route, routeUnregister);

  ++it;
  BOOST_CHECK(it == batch.end());
}

BOOST_AUTO_TEST_SUITE_END() // TestRibUpdateBatch
BOOST_AUTO_TEST_SUITE_END() // Rib

} // namespace nfd::tests
