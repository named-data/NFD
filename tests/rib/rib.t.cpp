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

namespace nfd {
namespace rib {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestRib, nfd::tests::BaseFixture)

BOOST_AUTO_TEST_CASE(RibEntry)
{
  rib::RibEntry entry;

  rib::Route route1;
  route1.faceId = 1;
  route1.origin = 0;

  entry.insertRoute(route1);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 1);

  Route route2;
  route2.faceId = 1;
  route2.origin = 128;

  entry.insertRoute(route2);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 2);

  entry.eraseRoute(route1);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 1);

  BOOST_CHECK(entry.findRoute(route1) == entry.getRoutes().end());
  BOOST_CHECK(entry.findRoute(route2) != entry.getRoutes().end());

  entry.insertRoute(route2);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 1);

  entry.eraseRoute(route1);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 1);
  BOOST_CHECK(entry.findRoute(route2) != entry.getRoutes().end());
}

BOOST_AUTO_TEST_CASE(Parent)
{
  rib::Rib rib;

  Route root;
  Name name1("/");
  root.faceId = 1;
  root.origin = 20;
  rib.insert(name1, root);

  Route route1;
  Name name2("/hello");
  route1.faceId = 2;
  route1.origin = 20;
  rib.insert(name2, route1);

  Route route2;
  Name name3("/hello/world");
  route2.faceId = 3;
  route2.origin = 20;
  rib.insert(name3, route2);

  shared_ptr<rib::RibEntry> ribEntry = rib.findParent(name3);
  BOOST_REQUIRE(static_cast<bool>(ribEntry));
  BOOST_CHECK_EQUAL(ribEntry->getRoutes().front().faceId, 2);

  ribEntry = rib.findParent(name2);
  BOOST_REQUIRE(static_cast<bool>(ribEntry));
  BOOST_CHECK_EQUAL(ribEntry->getRoutes().front().faceId, 1);

  Route route3;
  Name name4("/hello/test/foo/bar");
  route2.faceId = 3;
  route2.origin = 20;
  rib.insert(name4, route3);

  ribEntry = rib.findParent(name4);
  BOOST_CHECK(ribEntry != shared_ptr<rib::RibEntry>());
  BOOST_CHECK(ribEntry->getRoutes().front().faceId == 2);
}

BOOST_AUTO_TEST_CASE(Children)
{
  rib::Rib rib;

  Route route1;
  Name name1("/");
  route1.faceId = 1;
  route1.origin = 20;
  rib.insert(name1, route1);

  Route route2;
  Name name2("/hello/world");
  route2.faceId = 2;
  route2.origin = 20;
  rib.insert(name2, route2);

  Route route3;
  Name name3("/hello/test/foo/bar");
  route3.faceId = 3;
  route3.origin = 20;
  rib.insert(name3, route3);

  BOOST_CHECK_EQUAL((rib.find(name1)->second)->getChildren().size(), 2);
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getChildren().size(), 0);
  BOOST_CHECK_EQUAL((rib.find(name3)->second)->getChildren().size(), 0);

  Route entry4;
  Name name4("/hello");
  entry4.faceId = 4;
  entry4.origin = 20;
  rib.insert(name4, entry4);

  BOOST_CHECK_EQUAL((rib.find(name1)->second)->getChildren().size(), 1);
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getChildren().size(), 0);
  BOOST_CHECK_EQUAL((rib.find(name3)->second)->getChildren().size(), 0);
  BOOST_CHECK_EQUAL((rib.find(name4)->second)->getChildren().size(), 2);

  BOOST_CHECK_EQUAL((rib.find(name1)->second)->getChildren().front()->getName(), "/hello");
  BOOST_CHECK_EQUAL((rib.find(name4)->second)->getParent()->getName(), "/");

  BOOST_REQUIRE(static_cast<bool>((rib.find(name2)->second)->getParent()));
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getParent()->getName(), name4);
  BOOST_REQUIRE(static_cast<bool>((rib.find(name3)->second)->getParent()));
  BOOST_CHECK_EQUAL((rib.find(name3)->second)->getParent()->getName(), name4);
}

BOOST_AUTO_TEST_CASE(EraseFace)
{
  rib::Rib rib;

  Route route1;
  Name name1("/");
  route1.faceId = 1;
  route1.origin = 20;
  rib.insert(name1, route1);

  Route route2;
  Name name2("/hello/world");
  route2.faceId = 2;
  route2.origin = 20;
  rib.insert(name2, route2);

  Route route3;
  Name name3("/hello/world");
  route3.faceId = 1;
  route3.origin = 20;
  rib.insert(name3, route3);

  Route entry4;
  Name name4("/not/inserted");
  entry4.faceId = 1;
  entry4.origin = 20;

  rib.erase(name4, entry4);
  rib.erase(name1, route1);

  BOOST_CHECK(rib.find(name1) == rib.end());
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getRoutes().size(), 2);

  rib.erase(name2, route2);

  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getRoutes().size(), 1);
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getRoutes().front().faceId, 1);

  rib.erase(name3, route3);

  BOOST_CHECK(rib.find(name2) == rib.end());

  rib.erase(name4, entry4);
}

BOOST_AUTO_TEST_CASE(EraseRibEntry)
{
  rib::Rib rib;

  Route route1;
  Name name1("/");
  route1.faceId = 1;
  route1.origin = 20;
  rib.insert(name1, route1);

  Route route2;
  Name name2("/hello");
  route2.faceId = 2;
  route2.origin = 20;
  rib.insert(name2, route2);

  Route route3;
  Name name3("/hello/world");
  route3.faceId = 1;
  route3.origin = 20;
  rib.insert(name3, route3);

  shared_ptr<rib::RibEntry> ribEntry1 = rib.find(name1)->second;
  shared_ptr<rib::RibEntry> ribEntry2 = rib.find(name2)->second;
  shared_ptr<rib::RibEntry> ribEntry3 = rib.find(name3)->second;

  BOOST_CHECK(ribEntry1->getChildren().front() == ribEntry2);
  BOOST_CHECK(ribEntry3->getParent() == ribEntry2);

  rib.erase(name2, route2);
  BOOST_CHECK(ribEntry1->getChildren().front() == ribEntry3);
  BOOST_CHECK(ribEntry3->getParent() == ribEntry1);
}

BOOST_AUTO_TEST_CASE(Basic)
{
  rib::Rib rib;

  Route route1;
  Name name1("/hello/world");
  route1.faceId = 1;
  route1.origin = 20;
  route1.cost = 10;
  route1.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT | ndn::nfd::ROUTE_FLAG_CAPTURE;
  route1.expires = time::steady_clock::now() + time::milliseconds(1500);

  rib.insert(name1, route1);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  rib.insert(name1, route1);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  Route route2;
  Name name2("/hello/world");
  route2.faceId = 1;
  route2.origin = 20;
  route2.cost = 100;
  route2.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;
  route2.expires = time::steady_clock::now() + time::seconds(0);

  rib.insert(name2, route2);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  route2.faceId = 2;
  rib.insert(name2, route2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  BOOST_CHECK(rib.find(name1)->second->hasFaceId(route1.faceId));
  BOOST_CHECK(rib.find(name1)->second->hasFaceId(route2.faceId));

  Name name3("/foo/bar");
  rib.insert(name3, route2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  route2.origin = 1;
  rib.insert(name3, route2);
  BOOST_CHECK_EQUAL(rib.size(), 4);

  rib.erase(name3, route2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  Name name4("/hello/world");
  rib.erase(name4, route2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  route2.origin = 20;
  rib.erase(name4, route2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  BOOST_CHECK_EQUAL(rib.find(name2, route2), static_cast<Route*>(0));
  BOOST_CHECK_NE(rib.find(name1, route1), static_cast<Route*>(0));

  rib.erase(name1, route1);
  BOOST_CHECK_EQUAL(rib.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace rib
} // namespace nfd
