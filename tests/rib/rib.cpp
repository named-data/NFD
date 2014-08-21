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

namespace nfd {
namespace rib {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(Rib, nfd::tests::BaseFixture)

BOOST_AUTO_TEST_CASE(RibEntry)
{
  rib::RibEntry entry;

  rib::FaceEntry face1;
  face1.faceId = 1;
  face1.origin = 0;

  entry.insertFace(face1);
  BOOST_CHECK_EQUAL(entry.getFaces().size(), 1);

  FaceEntry face2;
  face2.faceId = 1;
  face2.origin = 128;

  entry.insertFace(face2);
  BOOST_CHECK_EQUAL(entry.getFaces().size(), 2);

  entry.eraseFace(face1);
  BOOST_CHECK_EQUAL(entry.getFaces().size(), 1);

  BOOST_CHECK(entry.findFace(face1) == entry.getFaces().end());
  BOOST_CHECK(entry.findFace(face2) != entry.getFaces().end());

  entry.insertFace(face2);
  BOOST_CHECK_EQUAL(entry.getFaces().size(), 1);

  entry.eraseFace(face1);
  BOOST_CHECK_EQUAL(entry.getFaces().size(), 1);
  BOOST_CHECK(entry.findFace(face2) != entry.getFaces().end());
}

BOOST_AUTO_TEST_CASE(Parent)
{
  rib::Rib rib;

  FaceEntry root;
  Name name1("/");
  root.faceId = 1;
  root.origin = 20;
  rib.insert(name1, root);

  FaceEntry entry1;
  Name name2("/hello");
  entry1.faceId = 2;
  entry1.origin = 20;
  rib.insert(name2, entry1);

  FaceEntry entry2;
  Name name3("/hello/world");
  entry2.faceId = 3;
  entry2.origin = 20;
  rib.insert(name3, entry2);

  shared_ptr<rib::RibEntry> ribEntry = rib.findParent(name3);
  BOOST_REQUIRE(static_cast<bool>(ribEntry));
  BOOST_CHECK_EQUAL(ribEntry->getFaces().front().faceId, 2);

  ribEntry = rib.findParent(name2);
  BOOST_REQUIRE(static_cast<bool>(ribEntry));
  BOOST_CHECK_EQUAL(ribEntry->getFaces().front().faceId, 1);

  FaceEntry entry3;
  Name name4("/hello/test/foo/bar");
  entry2.faceId = 3;
  entry2.origin = 20;
  rib.insert(name4, entry3);

  ribEntry = rib.findParent(name4);
  BOOST_CHECK(ribEntry != shared_ptr<rib::RibEntry>());
  BOOST_CHECK(ribEntry->getFaces().front().faceId == 2);
}

BOOST_AUTO_TEST_CASE(Children)
{
  rib::Rib rib;

  FaceEntry entry1;
  Name name1("/");
  entry1.faceId = 1;
  entry1.origin = 20;
  rib.insert(name1, entry1);

  FaceEntry entry2;
  Name name2("/hello/world");
  entry2.faceId = 2;
  entry2.origin = 20;
  rib.insert(name2, entry2);

  FaceEntry entry3;
  Name name3("/hello/test/foo/bar");
  entry3.faceId = 3;
  entry3.origin = 20;
  rib.insert(name3, entry3);

  BOOST_CHECK_EQUAL((rib.find(name1)->second)->getChildren().size(), 2);
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getChildren().size(), 0);
  BOOST_CHECK_EQUAL((rib.find(name3)->second)->getChildren().size(), 0);

  FaceEntry entry4;
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

  FaceEntry entry1;
  Name name1("/");
  entry1.faceId = 1;
  entry1.origin = 20;
  rib.insert(name1, entry1);

  FaceEntry entry2;
  Name name2("/hello/world");
  entry2.faceId = 2;
  entry2.origin = 20;
  rib.insert(name2, entry2);

  FaceEntry entry3;
  Name name3("/hello/world");
  entry3.faceId = 1;
  entry3.origin = 20;
  rib.insert(name3, entry3);

  FaceEntry entry4;
  Name name4("/not/inserted");
  entry4.faceId = 1;
  entry4.origin = 20;

  rib.erase(name4, entry4);
  rib.erase(name1, entry1);

  BOOST_CHECK(rib.find(name1) == rib.end());
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getFaces().size(), 2);

  rib.erase(name2, entry2);

  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getFaces().size(), 1);
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getFaces().front().faceId, 1);

  rib.erase(name3, entry3);

  BOOST_CHECK(rib.find(name2) == rib.end());

  rib.erase(name4, entry4);
}

BOOST_AUTO_TEST_CASE(EraseRibEntry)
{
  rib::Rib rib;

  FaceEntry entry1;
  Name name1("/");
  entry1.faceId = 1;
  entry1.origin = 20;
  rib.insert(name1, entry1);

  FaceEntry entry2;
  Name name2("/hello");
  entry2.faceId = 2;
  entry2.origin = 20;
  rib.insert(name2, entry2);

  FaceEntry entry3;
  Name name3("/hello/world");
  entry3.faceId = 1;
  entry3.origin = 20;
  rib.insert(name3, entry3);

  shared_ptr<rib::RibEntry> ribEntry1 = rib.find(name1)->second;
  shared_ptr<rib::RibEntry> ribEntry2 = rib.find(name2)->second;
  shared_ptr<rib::RibEntry> ribEntry3 = rib.find(name3)->second;

  BOOST_CHECK(ribEntry1->getChildren().front() == ribEntry2);
  BOOST_CHECK(ribEntry3->getParent() == ribEntry2);

  rib.erase(name2, entry2);
  BOOST_CHECK(ribEntry1->getChildren().front() == ribEntry3);
  BOOST_CHECK(ribEntry3->getParent() == ribEntry1);
}

BOOST_AUTO_TEST_CASE(EraseByFaceId)
{
  rib::Rib rib;

  FaceEntry entry1;
  Name name1("/");
  entry1.faceId = 1;
  entry1.origin = 20;
  rib.insert(name1, entry1);

  FaceEntry entry2;
  Name name2("/hello/world");
  entry2.faceId = 2;
  entry2.origin = 20;
  rib.insert(name2, entry2);

  FaceEntry entry3;
  Name name3("/hello/world");
  entry3.faceId = 1;
  entry3.origin = 20;
  rib.insert(name3, entry3);

  rib.erase(1);
  BOOST_CHECK(rib.find(name1) == rib.end());
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getFaces().size(), 1);

  rib.erase(3);
  BOOST_CHECK_EQUAL((rib.find(name2)->second)->getFaces().size(), 1);

  rib.erase(2);
  BOOST_CHECK(rib.find(name2) == rib.end());

  rib.erase(3);
}

BOOST_AUTO_TEST_CASE(Basic)
{
  rib::Rib rib;

  FaceEntry entry1;
  Name name1("/hello/world");
  entry1.faceId = 1;
  entry1.origin = 20;
  entry1.cost = 10;
  entry1.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT | ndn::nfd::ROUTE_FLAG_CAPTURE;
  entry1.expires = time::steady_clock::now() + time::milliseconds(1500);

  rib.insert(name1, entry1);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  rib.insert(name1, entry1);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  FaceEntry entry2;
  Name name2("/hello/world");
  entry2.faceId = 1;
  entry2.origin = 20;
  entry2.cost = 100;
  entry2.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;
  entry2.expires = time::steady_clock::now() + time::seconds(0);

  rib.insert(name2, entry2);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  entry2.faceId = 2;
  rib.insert(name2, entry2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  BOOST_CHECK(rib.find(name1)->second->hasFaceId(entry1.faceId));
  BOOST_CHECK(rib.find(name1)->second->hasFaceId(entry2.faceId));

  Name name3("/foo/bar");
  rib.insert(name3, entry2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  entry2.origin = 1;
  rib.insert(name3, entry2);
  BOOST_CHECK_EQUAL(rib.size(), 4);

  rib.erase(name3, entry2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  Name name4("/hello/world");
  rib.erase(name4, entry2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  entry2.origin = 20;
  rib.erase(name4, entry2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  BOOST_CHECK_EQUAL(rib.find(name2, entry2), static_cast<FaceEntry*>(0));
  BOOST_CHECK_NE(rib.find(name1, entry1), static_cast<FaceEntry*>(0));

  rib.erase(name1, entry1);
  BOOST_CHECK_EQUAL(rib.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace rib
} // namespace nfd
