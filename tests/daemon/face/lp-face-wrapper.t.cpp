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

#include "face/lp-face-wrapper.hpp"
#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "dummy-lp-face.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestLpFaceWrapper, BaseFixture)

BOOST_AUTO_TEST_CASE(SetId)
{
  Forwarder forwarder;
  auto face1w = make_shared<face::LpFaceWrapper>(make_unique<DummyLpFace>());
  auto face1 = static_cast<DummyLpFace*>(face1w->getLpFace());

  BOOST_CHECK_EQUAL(face1->getId(), nfd::face::INVALID_FACEID);
  BOOST_CHECK_EQUAL(face1w->getId(), nfd::INVALID_FACEID);

  forwarder.addFace(face1w);

  BOOST_CHECK_NE(face1->getId(), nfd::face::INVALID_FACEID);
  BOOST_CHECK_NE(face1w->getId(), nfd::INVALID_FACEID);
  BOOST_CHECK_EQUAL(face1->getId(), static_cast<face::FaceId>(face1w->getId()));
}

BOOST_AUTO_TEST_CASE(SetPersistency)
{
  unique_ptr<LpFace> face1u = make_unique<DummyLpFace>();
  face1u->setPersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);

  auto face1w = make_shared<face::LpFaceWrapper>(std::move(face1u));
  auto face1 = static_cast<DummyLpFace*>(face1w->getLpFace());

  BOOST_CHECK_EQUAL(face1->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(face1w->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);

  face1w->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  BOOST_CHECK_EQUAL(face1->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(face1w->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
}

BOOST_AUTO_TEST_CASE(FailSignal)
{
  auto face1w = make_shared<face::LpFaceWrapper>(make_unique<DummyLpFace>());
  auto face1 = static_cast<DummyLpFace*>(face1w->getLpFace());

  bool isFailed = false;
  face1w->onFail.connect(bind([&isFailed] { isFailed = true; }));

  face1->setState(FaceState::DOWN);
  BOOST_CHECK(!isFailed);

  face1->setState(FaceState::FAILED);
  BOOST_CHECK(!isFailed);

  face1->setState(FaceState::CLOSED);
  BOOST_CHECK(isFailed);
}

BOOST_AUTO_TEST_CASE(SendReceive)
{
  auto face1w = make_shared<face::LpFaceWrapper>(make_unique<DummyLpFace>());
  auto face1 = static_cast<DummyLpFace*>(face1w->getLpFace());

  const size_t nInInterests = 192;
  const size_t nInData = 91;
  const size_t nInNacks = 29;
  const size_t nOutInterests = 202;
  const size_t nOutData = 128;
  const size_t nOutNacks = 84;

  size_t nReceivedInterests = 0;
  size_t nReceivedData = 0;
  size_t nReceivedNacks = 0;
  face1w->onReceiveInterest.connect(bind([&nReceivedInterests] { ++nReceivedInterests; }));
  face1w->onReceiveData.connect(bind([&nReceivedData] { ++nReceivedData; }));
  face1w->onReceiveNack.connect(bind([&nReceivedNacks] { ++nReceivedNacks; }));

  BOOST_CHECK_EQUAL(face1->getCounters().getNInInterests(), 0);
  BOOST_CHECK_EQUAL(face1->getCounters().getNInDatas(), 0);
  BOOST_CHECK_EQUAL(face1->getCounters().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(face1->getCounters().getNOutDatas(), 0);
  BOOST_CHECK_EQUAL(face1w->getCounters().getNInInterests(), 0);
  BOOST_CHECK_EQUAL(face1w->getCounters().getNInDatas(), 0);
  BOOST_CHECK_EQUAL(face1w->getCounters().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(face1w->getCounters().getNOutDatas(), 0);
  // There's no counters for NACK for now.

  for (size_t i = 0; i < nInInterests; ++i) {
    shared_ptr<Interest> interest = makeInterest("/JSQdqward4");
    face1->receiveInterest(*interest);
  }

  for (size_t i = 0; i < nInData; ++i) {
    shared_ptr<Data> data = makeData("/hT8FDigWn1");
    face1->receiveData(*data);
  }

  for (size_t i = 0; i < nInNacks; ++i) {
    lp::Nack nack = makeNack("/StnEVTj4Ex", 561, lp::NackReason::CONGESTION);
    face1->receiveNack(nack);
  }

  for (size_t i = 0; i < nOutInterests; ++i) {
    shared_ptr<Interest> interest = makeInterest("/XyUAFYQDmd");
    face1w->sendInterest(*interest);
  }

  for (size_t i = 0; i < nOutData; ++i) {
    shared_ptr<Data> data = makeData("/GigPEtPH6");
    face1w->sendData(*data);
  }

  for (size_t i = 0; i < nOutNacks; ++i) {
    lp::Nack nack = makeNack("/9xK6FbwIBM", 365, lp::NackReason::CONGESTION);
    face1w->sendNack(nack);
  }

  BOOST_CHECK_EQUAL(face1->getCounters().getNInInterests(), nInInterests);
  BOOST_CHECK_EQUAL(face1->getCounters().getNInDatas(), nInData);
  BOOST_CHECK_EQUAL(face1->getCounters().getNOutInterests(), nOutInterests);
  BOOST_CHECK_EQUAL(face1->getCounters().getNOutDatas(), nOutData);
  BOOST_CHECK_EQUAL(face1w->getCounters().getNInInterests(), nInInterests);
  BOOST_CHECK_EQUAL(face1w->getCounters().getNInDatas(), nInData);
  BOOST_CHECK_EQUAL(face1w->getCounters().getNOutInterests(), nOutInterests);
  BOOST_CHECK_EQUAL(face1w->getCounters().getNOutDatas(), nOutData);

  BOOST_CHECK_EQUAL(nReceivedInterests, nInInterests);
  BOOST_CHECK_EQUAL(nReceivedData, nInData);
  BOOST_CHECK_EQUAL(nReceivedNacks, nInNacks);
  BOOST_CHECK_EQUAL(face1->sentInterests.size(), nOutInterests);
  BOOST_CHECK_EQUAL(face1->sentData.size(), nOutData);
  BOOST_CHECK_EQUAL(face1->sentNacks.size(), nOutNacks);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace face
} // namespace nfd
