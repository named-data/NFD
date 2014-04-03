/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/face-flags.hpp"

#include "tests/test-common.hpp"
#include "tests/face/dummy-face.hpp"

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
