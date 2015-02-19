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

#include "fw/face-table.hpp"
#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwFaceTable, BaseFixture)

BOOST_AUTO_TEST_CASE(AddRemove)
{
  Forwarder forwarder;

  FaceTable& faceTable = forwarder.getFaceTable();
  std::vector<FaceId> onAddHistory;
  std::vector<FaceId> onRemoveHistory;
  faceTable.onAdd.connect([&] (shared_ptr<Face> face) {
    onAddHistory.push_back(face->getId());
  });
  faceTable.onRemove.connect([&] (shared_ptr<Face> face) {
    onRemoveHistory.push_back(face->getId());
  });

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();

  BOOST_CHECK_EQUAL(face1->getId(), INVALID_FACEID);
  BOOST_CHECK_EQUAL(face2->getId(), INVALID_FACEID);

  forwarder.addFace(face1);
  forwarder.addFace(face2);

  BOOST_CHECK_NE(face1->getId(), INVALID_FACEID);
  BOOST_CHECK_NE(face2->getId(), INVALID_FACEID);
  BOOST_CHECK_NE(face1->getId(), face2->getId());
  BOOST_CHECK_GT(face1->getId(), FACEID_RESERVED_MAX);
  BOOST_CHECK_GT(face2->getId(), FACEID_RESERVED_MAX);

  FaceId oldId1 = face1->getId();
  faceTable.add(face1);
  BOOST_CHECK_EQUAL(face1->getId(), oldId1);
  BOOST_CHECK_EQUAL(faceTable.size(), 2);

  BOOST_REQUIRE_EQUAL(onAddHistory.size(), 2);
  BOOST_CHECK_EQUAL(onAddHistory[0], face1->getId());
  BOOST_CHECK_EQUAL(onAddHistory[1], face2->getId());

  face1->close();

  BOOST_CHECK_EQUAL(face1->getId(), INVALID_FACEID);

  BOOST_REQUIRE_EQUAL(onRemoveHistory.size(), 1);
  BOOST_CHECK_EQUAL(onRemoveHistory[0], onAddHistory[0]);
}

BOOST_AUTO_TEST_CASE(AddReserved)
{
  Forwarder forwarder;
  FaceTable& faceTable = forwarder.getFaceTable();

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  BOOST_CHECK_EQUAL(face1->getId(), INVALID_FACEID);

  faceTable.addReserved(face1, 5);
  BOOST_CHECK_EQUAL(face1->getId(), 5);
}

BOOST_AUTO_TEST_CASE(Enumerate)
{
  Forwarder forwarder;
  FaceTable& faceTable = forwarder.getFaceTable();

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
    if (*it == face1) {
      hasFace1 = true;
    }
  }
  BOOST_CHECK(hasFace1);

  faceTable.add(face2);
  BOOST_CHECK_EQUAL(faceTable.size(), 2);
  BOOST_CHECK_EQUAL(std::distance(faceTable.begin(), faceTable.end()), faceTable.size());
  hasFace1 = hasFace2 = false;
  for (FaceTable::const_iterator it = faceTable.begin(); it != faceTable.end(); ++it) {
    if (*it == face1) {
      hasFace1 = true;
    }
    if (*it == face2) {
      hasFace2 = true;
    }
  }
  BOOST_CHECK(hasFace1);
  BOOST_CHECK(hasFace2);

  face1->close();
  BOOST_CHECK_EQUAL(faceTable.size(), 1);
  BOOST_CHECK_EQUAL(std::distance(faceTable.begin(), faceTable.end()), faceTable.size());
  hasFace1 = hasFace2 = false;
  for (FaceTable::const_iterator it = faceTable.begin(); it != faceTable.end(); ++it) {
    if (*it == face1) {
      hasFace1 = true;
    }
    if (*it == face2) {
      hasFace2 = true;
    }
  }
  BOOST_CHECK(!hasFace1);
  BOOST_CHECK(hasFace2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
