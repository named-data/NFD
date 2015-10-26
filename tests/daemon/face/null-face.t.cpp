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

#include "face/null-face.hpp"
#include "face/lp-face-wrapper.hpp"
#include "transport-properties.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestNullFace, BaseFixture)

using nfd::Face;

BOOST_AUTO_TEST_CASE(StaticProperties)
{
  shared_ptr<Face> faceW = makeNullFace(FaceUri("testnull://hhppt12sy"));
  LpFace* face = static_pointer_cast<LpFaceWrapper>(faceW)->getLpFace();
  checkStaticPropertiesInitialized(*face->getTransport());

  BOOST_CHECK_EQUAL(face->getLocalUri(), FaceUri("testnull://hhppt12sy"));
  BOOST_CHECK_EQUAL(face->getRemoteUri(), FaceUri("testnull://hhppt12sy"));
  BOOST_CHECK_EQUAL(face->getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
}

BOOST_AUTO_TEST_CASE(Send)
{
  shared_ptr<Face> face = makeNullFace();

  shared_ptr<Interest> interest = makeInterest("/A");
  BOOST_CHECK_NO_THROW(face->sendInterest(*interest));

  shared_ptr<Data> data = makeData("/B");
  BOOST_CHECK_NO_THROW(face->sendData(*data));

  BOOST_CHECK_NO_THROW(face->close());
}

BOOST_AUTO_TEST_SUITE_END() // TestNullFace
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
