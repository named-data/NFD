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

class FaceFailTestFace : public DummyFace
{
public:
  FaceFailTestFace()
    : failCount(0)
  {
    this->onFail.connect(bind(&FaceFailTestFace::failHandler, this, _1));
  }

  void
  failOnce()
  {
    this->fail("reason");
  }

private:
  void
  failHandler(const std::string& reason)
  {
    BOOST_CHECK_EQUAL(reason, "reason");
    ++this->failCount;
  }

public:
  int failCount;
};

BOOST_AUTO_TEST_CASE(FailTwice)
{
  FaceFailTestFace face;
  BOOST_CHECK_EQUAL(face.failCount, 0);
  face.failOnce();
  BOOST_CHECK_EQUAL(face.failCount, 1);
  face.failOnce();
  BOOST_CHECK_EQUAL(face.failCount, 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
