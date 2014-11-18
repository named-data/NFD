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

#include "rib/rib.hpp"

#include "tests/test-common.hpp"
#include "fib-updates-common.hpp"

namespace nfd {
namespace rib {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestFibUpdates, FibUpdatesFixture)

BOOST_AUTO_TEST_SUITE(NewNamespace)

BOOST_AUTO_TEST_CASE(NoFlags)
{
  // No flags, empty RIB, should generate 1 update for the inserted route
  insertRoute("/a/b", 1, 0, 10, 0);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  // Reset RIB
  eraseRoute("/a/b", 1, 0);
  clearFibUpdates();

  // Parent with child inherit flag
  insertRoute("/a", 2, 0, 70, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 3, 0, 30, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 3 updates, 1 for the inserted route and 2 from inheritance
  insertRoute("/a/b", 1, 0, 10, 0);

  updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 3);

  update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 2);
  BOOST_CHECK_EQUAL(update->cost, 70);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 3);
  BOOST_CHECK_EQUAL(update->cost, 30);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(BothFlags)
{
  // Empty RIB, should generate 1 update for the inserted route
  insertRoute("/a", 1, 0, 10, (ndn::nfd::ROUTE_FLAG_CHILD_INHERIT | ndn::nfd::ROUTE_FLAG_CAPTURE));

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  // Reset RIB
  eraseRoute("/a", 1, 0);
  clearFibUpdates();

  insertRoute("/", 2, 0, 70, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a/b", 3, 0, 30, 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 3 updates, 1 for the inserted route, 1 to add the route to the child,
  // and 1 to remove the previously inherited route
  insertRoute("/a", 1, 0, 10, (ndn::nfd::ROUTE_FLAG_CHILD_INHERIT | ndn::nfd::ROUTE_FLAG_CAPTURE));

  updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 3);

  update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 2);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(ChildInherit)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a/b", 2, 0, 10, 0);
  insertRoute("/a/c", 3, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 1 for the inserted route and 1 to add the route to "/a/b"
  insertRoute("/a", 1, 0, 10, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(Capture)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a/b", 2, 0, 10, 0);
  insertRoute("/a/c", 3, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 1 for the inserted route and
  // 1 to remove the inherited route from "/a/b"
  insertRoute("/a", 1, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_SUITE_END() // NewNamespace

BOOST_AUTO_TEST_SUITE_END() // FibUpdates

} // namespace tests
} // namespace rib
} // namespace nfd
