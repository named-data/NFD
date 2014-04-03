/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/face.hpp"
#include "face/local-face.hpp"
#include "dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceFace, BaseFixture)

BOOST_AUTO_TEST_CASE(Description)
{
  DummyFace face;
  face.setDescription("3pFsKrvWr");
  BOOST_CHECK_EQUAL(face.getDescription(), "3pFsKrvWr");
}

BOOST_AUTO_TEST_CASE(LocalControlHeaderEnabled)
{
  DummyLocalFace face;

  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(), false);

  face.setLocalControlHeaderFeature(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID, true);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(), true);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID), true);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(
                         LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID), false);

  face.setLocalControlHeaderFeature(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID, false);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(), false);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(
                         LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID), false);
}

BOOST_AUTO_TEST_CASE(Counters)
{
  DummyFace face;
  const FaceCounters& counters = face.getCounters();
  BOOST_CHECK_EQUAL(counters.getNInInterests() , 0);
  BOOST_CHECK_EQUAL(counters.getNInDatas()     , 0);
  BOOST_CHECK_EQUAL(counters.getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(counters.getNOutDatas()    , 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
