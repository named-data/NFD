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

#include "rib/rib.hpp"

#include "tests/test-common.hpp"
#include "fib-updates-common.hpp"

namespace nfd {
namespace rib {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestFibUpdates, FibUpdatesFixture)

BOOST_AUTO_TEST_SUITE(EraseFace)

BOOST_AUTO_TEST_CASE(WithInheritedFace_Root)
{
  insertRoute("/", 1, 0, 10, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a/b", 2, 0, 75, 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 1 updates: 1 to remove face 1 from /
  eraseRoute("/", 1, 0);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(WithInheritedFace)
{
  insertRoute("/a", 5, 0, 10, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 5, 255, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 20, 0);
  insertRoute("/a/b", 3, 0, 5, 0);

  // /a should have face 5 with cost 10; /a/b should have face 3 with cost 5 and
  // face 5 with cost 10
  eraseRoute("/a", 5, 255);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 1 to remove face 3 from /a/b and one to remove inherited route
  eraseRoute("/a/b", 3, 0);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 3);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 5);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(MultipleFaces)
{
  insertRoute("/a", 5, 0, 10, 0);
  insertRoute("/a", 5, 255, 5, 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 1 updates: 1 to update cost to 10 for /a
  eraseRoute("/a", 5, 255);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 5);
  BOOST_CHECK_EQUAL(update->cost, 10);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(NoFlags_NoCaptureChange_NoCaptureOnRoute)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, 0);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 1, 128, 50, 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 1 updates: 1 to update cost for /a
  eraseRoute("/a", 1, 128);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 5);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(MakeRibEmpty)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 1 updates: 1 to remove route from /
  eraseRoute("/", 1, 0);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(NoFlags_NoCaptureChange_CaptureOnRoute)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 1, 128, 50, 0);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 1 updates: 1 to remove route from /a
  eraseRoute("/a", 1, 128);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(BothFlags_NoCaptureChange_CaptureOnRoute)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 1, 128, 50, (ndn::nfd::ROUTE_FLAG_CHILD_INHERIT |
                                     ndn::nfd::ROUTE_FLAG_CAPTURE));

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 1 to remove face1 from /a and
  // 1 to remove face1 from /a/b
  eraseRoute("/a", 1, 128);

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

BOOST_AUTO_TEST_CASE(BothFlags_CaptureChange_NoCaptureOnRoute)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, 0);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 1, 128, 50, (ndn::nfd::ROUTE_FLAG_CHILD_INHERIT |
                                     ndn::nfd::ROUTE_FLAG_CAPTURE));

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 1 to add face1 to /a and
  // 1 to add face1 to /a/b
  eraseRoute("/a", 1, 128);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 5);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 5);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(ChildInherit_NoCaptureChange_NoCaptureOnRoute)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, 0);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 1, 128, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 2 to add face1 to /a and /a/b
  eraseRoute("/a", 1, 128);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 5);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 5);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(ChildInherit_NoCaptureChange_CaptureOnRoute)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 1, 128, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 2 to remove face 1 from /a and /a/b
  eraseRoute("/a", 1, 128);

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

BOOST_AUTO_TEST_CASE(Capture_CaptureChange_NoCaptureOnRoute)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, 0);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 1, 128, 50, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 1 to update cost on /a and
  // 1 to add face1 to /a/b
  eraseRoute("/a", 1 ,128);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 5);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->cost, 5);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(Capture_NoCaptureChange_CaptureOnRoute)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 2, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);
  insertRoute("/a/b", 3, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 1, 128, 50, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 1 updates: 1 to remove route from /a
  eraseRoute("/a", 1, 128);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(EraseFaceById)
{
  insertRoute("/", 1, 0, 5, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertRoute("/a", 3, 0, 10, 0);
  insertRoute("/a/b", 4, 0, 10, 0);
  insertRoute("/a/c", 1, 0, 100, 0);
  insertRoute("/a", 2, 128, 50, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Should generate 2 updates: 2 to add face ID 1 to /a and /a/b
  destroyFace(2);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/a");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL(update->name,  "/a/b");
  BOOST_CHECK_EQUAL(update->faceId, 1);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(RemoveNamespaceWithAncestorFace) // Bug #2757
{
  uint64_t faceId = 263;

  // Register route back to laptop
  insertRoute("/", faceId, ndn::nfd::ROUTE_ORIGIN_STATIC, 0, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Register remote prefix
  insertRoute("/Z/A", faceId, ndn::nfd::ROUTE_ORIGIN_CLIENT, 15, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  // Unregister remote prefix
  // Should create an update to remove the remote prefix but should not create
  // an update to add the ancestor route to the remote prefix's namespace
  eraseRoute("/Z/A", faceId, ndn::nfd::ROUTE_ORIGIN_CLIENT);

  FibUpdater::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  FibUpdater::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL(update->name,  "/Z/A");
  BOOST_CHECK_EQUAL(update->faceId, faceId);
  BOOST_CHECK_EQUAL(update->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(RemoveNamespaceWithCapture) // Bug #3404
{
  insertRoute("/", 262, ndn::nfd::ROUTE_ORIGIN_STATIC, 0, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  uint64_t appFaceId = 264;
  ndn::Name ndnConPrefix("/ndn/edu/site/ndnrtc/user/username/streams/main_camera/low");
  insertRoute(ndnConPrefix, appFaceId, ndn::nfd::ROUTE_ORIGIN_CLIENT, 0,
              ndn::nfd::ROUTE_FLAG_CHILD_INHERIT | ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  clearFibUpdates();

  destroyFace(appFaceId);

  // FibUpdater should not generate any inherited routes for the erased RibEntry
  BOOST_CHECK_EQUAL(fibUpdater.m_inheritedRoutes.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // EraseFace

BOOST_AUTO_TEST_SUITE_END() // FibUpdates

} // namespace tests
} // namespace rib
} // namespace nfd
