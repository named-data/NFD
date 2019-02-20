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

BOOST_AUTO_TEST_SUITE(UpdateFace)

BOOST_AUTO_TEST_CASE(TurnOffChildInheritLowerCost)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, 0);
  insertRoute("/", 1, 128, 25, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 1 to update the cost of / face 1 to 50 and
  // 1 to update the cost of /a face 1 to 50
  insertRoute("/", 1, 128, 75, 0);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost,   50);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost,   50);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(UpdateOnLowerCostOnly)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, 0);
  insertRoute("/", 1, 128, 100, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 0 updates
  insertRoute("/", 1, 128, 75, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates
  insertRoute("/", 1, 128, 25, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost,   25);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost,   25);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(NoChangeInCost)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 100, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 4, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 0 updates
  insertRoute("/a", 2, 0, 100, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 0);
}

BOOST_AUTO_TEST_CASE(ChangeCost)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 100, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 4, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 1 to add face2 with new cost to /a and
  // 1 to add face2 with new cost to /a/b
  insertRoute("/a", 2, 0, 300, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 2);
  BOOST_CHECK_EQUAL(update->cost,   300);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 2);
  BOOST_CHECK_EQUAL(update->cost,   300);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(TurnOnChildInherit)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, 0);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 4, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Turn on child inherit flag for the entry in /a
  // Should generate 1 updates: 1 to add face to /a/b
  insertRoute("/a", 2, 0, 10, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 2);
  BOOST_CHECK_EQUAL(update->cost,   10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(TurnOffChildInherit)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 1, 0, 100, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a/b", 2, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 25, 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Turn off child inherit flag for the entry in /a
  // Should generate 1 update: 1 to add face1 to /a/b
  insertRoute("/a", 1, 0, 100, 0);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost,   50);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(TurnOnCapture)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, 0);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 10, 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Turn on capture flag for the entry in /a
  // Should generate 2 updates: 1 to remove face1 from /a and
  // 1 to remove face1 from /a/b
  insertRoute("/a", 2, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(TurnOffCapture)
{
  insertRoute("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 10, 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Turn off capture flag for the entry in /a
  // Should generate 2 updates: 1 to add face1 to /a and
  // 1 to add face1 to /a/b
  insertRoute("/a", 2, 0, 10, 0);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 50);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 50);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_SUITE_END() // UpdateFace

BOOST_AUTO_TEST_SUITE_END() // FibUpdates

} // namespace tests
} // namespace rib
} // namespace nfd
