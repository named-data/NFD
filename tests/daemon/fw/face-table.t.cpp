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

#include "fw/face-table.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestFaceTable, BaseFixture)

BOOST_AUTO_TEST_CASE(AddRemove)
{
  FaceTable faceTable;

  std::vector<FaceId> addHistory;
  std::vector<FaceId> removeHistory;
  faceTable.afterAdd.connect([&] (const Face& face) { addHistory.push_back(face.getId()); });
  faceTable.beforeRemove.connect([&] (const Face& face) { removeHistory.push_back(face.getId()); });

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();

  BOOST_CHECK_EQUAL(face1->getId(), face::INVALID_FACEID);
  BOOST_CHECK_EQUAL(face2->getId(), face::INVALID_FACEID);
  BOOST_CHECK(faceTable.get(face::INVALID_FACEID) == nullptr);
  BOOST_CHECK_EQUAL(faceTable.size(), 0);

  faceTable.add(face1);
  faceTable.add(face2);
  BOOST_CHECK_EQUAL(faceTable.size(), 2);

  BOOST_CHECK_NE(face1->getId(), face::INVALID_FACEID);
  BOOST_CHECK_NE(face2->getId(), face::INVALID_FACEID);
  BOOST_CHECK_NE(face1->getId(), face2->getId());
  BOOST_CHECK_GT(face1->getId(), face::FACEID_RESERVED_MAX);
  BOOST_CHECK_GT(face2->getId(), face::FACEID_RESERVED_MAX);
  BOOST_CHECK(faceTable.get(face1->getId()) == face1.get());
  BOOST_CHECK(faceTable.get(face2->getId()) == face2.get());

  FaceId oldId1 = face1->getId();
  faceTable.add(face1);
  BOOST_CHECK_EQUAL(face1->getId(), oldId1);
  BOOST_CHECK_EQUAL(faceTable.size(), 2);

  BOOST_REQUIRE_EQUAL(addHistory.size(), 2);
  BOOST_CHECK_EQUAL(addHistory[0], face1->getId());
  BOOST_CHECK_EQUAL(addHistory[1], face2->getId());

  face1->close();

  BOOST_CHECK_EQUAL(face1->getId(), face::INVALID_FACEID);
  BOOST_CHECK_EQUAL(faceTable.size(), 1);
  BOOST_CHECK(faceTable.get(oldId1) == nullptr);

  BOOST_REQUIRE_EQUAL(removeHistory.size(), 1);
  BOOST_CHECK_EQUAL(removeHistory[0], addHistory[0]);
}

BOOST_AUTO_TEST_CASE(AddReserved)
{
  FaceTable faceTable;

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  BOOST_CHECK_EQUAL(face1->getId(), face::INVALID_FACEID);

  faceTable.addReserved(face1, 5);
  BOOST_CHECK_EQUAL(face1->getId(), 5);
}

BOOST_AUTO_TEST_CASE(Enumerate)
{
  FaceTable faceTable;

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  bool hasFace1 = false, hasFace2 = false;

  BOOST_CHECK_EQUAL(faceTable.size(), 0);
  BOOST_CHECK_EQUAL(std::distance(faceTable.begin(), faceTable.end()), faceTable.size());

  faceTable.add(face1);
  BOOST_CHECK_EQUAL(faceTable.size(), 1);
  BOOST_CHECK_EQUAL(std::distance(faceTable.begin(), faceTable.end()), faceTable.size());
  hasFace1 = hasFace2 = false;
  for (FaceTable::const_iterator it = faceTable.begin(); it != faceTable.end(); ++it) {
    hasFace1 = hasFace1 || &*it == face1.get();
  }
  BOOST_CHECK_EQUAL(hasFace1, true);

  faceTable.add(face2);
  BOOST_CHECK_EQUAL(faceTable.size(), 2);
  BOOST_CHECK_EQUAL(std::distance(faceTable.begin(), faceTable.end()), faceTable.size());
  hasFace1 = hasFace2 = false;
  for (FaceTable::const_iterator it = faceTable.begin(); it != faceTable.end(); ++it) {
    hasFace1 = hasFace1 || &*it == face1.get();
    hasFace2 = hasFace2 || &*it == face2.get();
  }
  BOOST_CHECK_EQUAL(hasFace1, true);
  BOOST_CHECK_EQUAL(hasFace2, true);

  face1->close();
  BOOST_CHECK_EQUAL(faceTable.size(), 1);
  BOOST_CHECK_EQUAL(std::distance(faceTable.begin(), faceTable.end()), faceTable.size());
  hasFace1 = hasFace2 = false;
  for (FaceTable::const_iterator it = faceTable.begin(); it != faceTable.end(); ++it) {
    hasFace1 = hasFace1 || &*it == face1.get();
    hasFace2 = hasFace2 || &*it == face2.get();
  }
  BOOST_CHECK_EQUAL(hasFace1, false);
  BOOST_CHECK_EQUAL(hasFace2, true);
}

BOOST_AUTO_TEST_SUITE_END() // TestFaceTable
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace nfd
