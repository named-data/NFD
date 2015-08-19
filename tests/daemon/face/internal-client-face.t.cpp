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

#include "face/internal-client-face.hpp"
#include "face/internal-face.hpp"
#include "tests/test-common.hpp"
#include "tests/identity-management-fixture.hpp"

namespace nfd {
namespace tests {

class InternalClientFaceFixture : public UnitTestTimeFixture
                                , public IdentityManagementFixture
{
public:
  InternalClientFaceFixture()
    : m_internalFace(make_shared<InternalFace>())
    , m_face(makeInternalClientFace(m_internalFace, m_keyChain))
  {
  }

protected:
  shared_ptr<InternalFace> m_internalFace;
  shared_ptr<InternalClientFace> m_face;
};

BOOST_FIXTURE_TEST_SUITE(FaceInternalClientFace, InternalClientFaceFixture)

BOOST_AUTO_TEST_CASE(ExpressInterest)
{
  bool didReceiveDataBack = false;
  bool didTimeoutCallbackFire = false;
  Data receivedData;

  auto expressInterest = [&] (shared_ptr<Interest> interest) {
    didReceiveDataBack = false;
    didTimeoutCallbackFire = false;
    m_face->expressInterest(interest->setInterestLifetime(time::milliseconds(100)),
                            [&] (const Interest& interest, Data& data) {
                              didReceiveDataBack = true;
                              receivedData = data;
                            },
                            bind([&] { didTimeoutCallbackFire = true; }));
    advanceClocks(time::milliseconds(1));
  };

  expressInterest(makeInterest("/test/timeout"));
  advanceClocks(time::milliseconds(1), 200); // wait for time out
  BOOST_CHECK(didTimeoutCallbackFire);

  auto dataToSend = makeData("/test/data")->setContent(Block("\x81\x01\0x01", 3));
  expressInterest(makeInterest("/test/data"));
  m_internalFace->sendData(dataToSend); // send data to satisfy the expressed interest
  advanceClocks(time::milliseconds(1));
  BOOST_CHECK(didReceiveDataBack);
  BOOST_CHECK(receivedData.wireEncode() == dataToSend.wireEncode());
}

BOOST_AUTO_TEST_CASE(InterestFilter)
{
  bool didOnInterestCallbackFire = false;
  Block receivedBlock, expectedBlock;

  auto testSendInterest = [&] (shared_ptr<Interest> interest) {
    didOnInterestCallbackFire = false;
    expectedBlock = interest->wireEncode();
    m_internalFace->sendInterest(*interest);
  };

  testSendInterest(makeInterest("/test/filter")); // no filter is set now
  BOOST_CHECK(!didOnInterestCallbackFire);

  // set filter
  auto filter = m_face->setInterestFilter("/test/filter",
                                          bind([&](const Interest& interset) {
                                              didOnInterestCallbackFire = true;
                                              receivedBlock = interset.wireEncode();
                                            }, _2));
  advanceClocks(time::milliseconds(1));

  testSendInterest(makeInterest("/test/filter"));
  BOOST_CHECK(didOnInterestCallbackFire);
  BOOST_CHECK(receivedBlock == expectedBlock);

  // unset filter
  didOnInterestCallbackFire = false;
  m_face->unsetInterestFilter(filter);
  advanceClocks(time::milliseconds(1));
  testSendInterest(makeInterest("/test/filter"));
  BOOST_CHECK(!didOnInterestCallbackFire);
}

BOOST_AUTO_TEST_CASE(PutData)
{
  bool didInternalFaceReceiveData = false;
  Data receivedData;
  m_internalFace->onReceiveData.connect([&] (const Data& data) {
      didInternalFaceReceiveData = true;
      receivedData = data;
    });

  auto dataToPut = makeData("/test/put/data");
  m_face->put(*dataToPut);
  advanceClocks(time::milliseconds(1));
  BOOST_CHECK(didInternalFaceReceiveData);
  BOOST_CHECK(receivedData.wireEncode() == dataToPut->wireEncode());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
