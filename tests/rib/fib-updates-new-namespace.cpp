/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

BOOST_FIXTURE_TEST_SUITE(FibUpdates, FibUpdatesFixture)

BOOST_AUTO_TEST_SUITE(NewNamespace)

BOOST_AUTO_TEST_CASE(NoFlags)
{
  // No flags, empty RIB, should generate 1 update for the inserted face
  insertFaceEntry("/a/b", 1, 0, 10, 0);

  Rib::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  Rib::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL((*update)->name,  "/a/b");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->cost, 10);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);

  // Reset RIB
  eraseFaceEntry("/a/b", 1, 0);
  rib.clearFibUpdates();

  // Parent with child inherit flag
  insertFaceEntry("/a", 2, 0, 70, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertFaceEntry("/a", 3, 0, 30, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  // Clear updates generated from previous insertions
  rib.clearFibUpdates();

  // Should generate 3 updates, 1 for the inserted face and 2 from inheritance
  insertFaceEntry("/a/b", 1, 0, 10, 0);

  updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 3);

  update = updates.begin();
  BOOST_CHECK_EQUAL((*update)->name,  "/a/b");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->cost, 10);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL((*update)->name,  "/a/b");
  BOOST_CHECK_EQUAL((*update)->faceId, 2);
  BOOST_CHECK_EQUAL((*update)->cost, 70);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL((*update)->name,  "/a/b");
  BOOST_CHECK_EQUAL((*update)->faceId, 3);
  BOOST_CHECK_EQUAL((*update)->cost, 30);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(BothFlags)
{
  // Empty RIB, should generate 1 update for the inserted face
  insertFaceEntry("/a", 1, 0, 10, (ndn::nfd::ROUTE_FLAG_CHILD_INHERIT |
                                   ndn::nfd::ROUTE_FLAG_CAPTURE));

  Rib::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 1);

  Rib::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL((*update)->name,  "/a");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->cost, 10);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);

  // Reset RIB
  eraseFaceEntry("/a", 1, 0);
  rib.clearFibUpdates();

  insertFaceEntry("/", 2, 0, 70, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertFaceEntry("/a/b", 3, 0, 30, 0);

  // Clear updates generated from previous insertions
  rib.clearFibUpdates();

  // Should generate 3 updates, 1 for the inserted face, 1 to add the face to the child,
  // and 1 to remove the previously inherited entry
  insertFaceEntry("/a", 1, 0, 10, (ndn::nfd::ROUTE_FLAG_CHILD_INHERIT |
                                   ndn::nfd::ROUTE_FLAG_CAPTURE));

  updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 3);

  update = updates.begin();
  BOOST_CHECK_EQUAL((*update)->name,  "/a");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->cost, 10);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL((*update)->name,  "/a/b");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->cost, 10);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL((*update)->name,  "/a/b");
  BOOST_CHECK_EQUAL((*update)->faceId, 2);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(ChildInherit)
{
  insertFaceEntry("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertFaceEntry("/a/b", 2, 0, 10, 0);
  insertFaceEntry("/a/c", 3, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  rib.clearFibUpdates();

  // Should generate 2 updates: 1 for the inserted face and 1 to add the face to "/a/b"
  insertFaceEntry("/a", 1, 0, 10, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);

  Rib::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  Rib::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL((*update)->name,  "/a");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->cost, 10);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL((*update)->name,  "/a/b");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->cost, 10);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);
}

BOOST_AUTO_TEST_CASE(Capture)
{
  insertFaceEntry("/", 1, 0, 50, ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  insertFaceEntry("/a/b", 2, 0, 10, 0);
  insertFaceEntry("/a/c", 3, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  // Clear updates generated from previous insertions
  rib.clearFibUpdates();

  // Should generate 2 updates: 1 for the inserted face and
  // 1 to remove inherited face from "/a/b"
  insertFaceEntry("/a", 1, 0, 10, ndn::nfd::ROUTE_FLAG_CAPTURE);

  Rib::FibUpdateList updates = getSortedFibUpdates();
  BOOST_REQUIRE_EQUAL(updates.size(), 2);

  Rib::FibUpdateList::const_iterator update = updates.begin();
  BOOST_CHECK_EQUAL((*update)->name,  "/a");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->cost, 10);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::ADD_NEXTHOP);

  ++update;
  BOOST_CHECK_EQUAL((*update)->name,  "/a/b");
  BOOST_CHECK_EQUAL((*update)->faceId, 1);
  BOOST_CHECK_EQUAL((*update)->action, FibUpdate::REMOVE_NEXTHOP);
}

BOOST_AUTO_TEST_SUITE_END() // NewNamespace

BOOST_AUTO_TEST_SUITE_END() // FibUpdates

} // namespace tests
} // namespace rib
} // namespace nfd
