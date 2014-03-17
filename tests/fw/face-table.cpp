/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fw/face-table.hpp"
#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "tests/face/dummy-face.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwFaceTable, BaseFixture)

BOOST_AUTO_TEST_CASE(AddRemove)
{
  Forwarder forwarder;

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();

  BOOST_CHECK_EQUAL(face1->getId(), INVALID_FACEID);
  BOOST_CHECK_EQUAL(face2->getId(), INVALID_FACEID);

  forwarder.addFace(face1);
  forwarder.addFace(face2);

  BOOST_CHECK_NE(face1->getId(), INVALID_FACEID);
  BOOST_CHECK_NE(face2->getId(), INVALID_FACEID);
  BOOST_CHECK_NE(face1->getId(), face2->getId());

  face1->close();
  face2->close();

  BOOST_CHECK_EQUAL(face1->getId(), INVALID_FACEID);
  BOOST_CHECK_EQUAL(face2->getId(), INVALID_FACEID);
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
