/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "mgmt/face-flags.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(MgmtFaceFlags, BaseFixture)

template<typename DummyFaceBase>
class DummyOnDemandFace : public DummyFaceBase
{
public:
  DummyOnDemandFace()
  {
    this->setOnDemand(true);
  }
};

BOOST_AUTO_TEST_CASE(Get)
{
  DummyFace face1;
  BOOST_CHECK_EQUAL(getFaceFlags(face1), 0);

  DummyLocalFace face2;
  BOOST_CHECK_EQUAL(getFaceFlags(face2), static_cast<uint64_t>(ndn::nfd::FACE_IS_LOCAL));

  DummyOnDemandFace<DummyFace> face3;
  BOOST_CHECK_EQUAL(getFaceFlags(face3), static_cast<uint64_t>(ndn::nfd::FACE_IS_ON_DEMAND));

  DummyOnDemandFace<DummyLocalFace> face4;
  BOOST_CHECK_EQUAL(getFaceFlags(face4), static_cast<uint64_t>(ndn::nfd::FACE_IS_LOCAL |
                                         ndn::nfd::FACE_IS_ON_DEMAND));

}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
