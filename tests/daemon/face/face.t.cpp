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

#include "face/face.hpp"

#include "tests/test-common.hpp"
#include "dummy-face.hpp"
#include "dummy-transport.hpp"
#include "face/generic-link-service.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestFace, BaseFixture)

using nfd::Face;

BOOST_AUTO_TEST_CASE(GetLinkService)
{
  auto linkService = make_unique<GenericLinkService>();
  auto linkServicePtr = linkService.get();
  auto face = make_unique<Face>(std::move(linkService), make_unique<DummyTransport>());

  BOOST_CHECK_EQUAL(face->getLinkService(), linkServicePtr);
}

BOOST_AUTO_TEST_CASE(GetTransport)
{
  auto dummyTransport = make_unique<DummyTransport>();
  auto transportPtr = dummyTransport.get();
  auto face = make_unique<Face>(make_unique<GenericLinkService>(), std::move(dummyTransport));

  BOOST_CHECK_EQUAL(face->getTransport(), transportPtr);
}

BOOST_AUTO_TEST_CASE(StaticProperties)
{
  auto face = make_unique<DummyFace>();

  BOOST_CHECK_EQUAL(face->getId(), INVALID_FACEID);
  BOOST_CHECK_EQUAL(face->getLocalUri().getScheme(), "dummy");
  BOOST_CHECK_EQUAL(face->getRemoteUri().getScheme(), "dummy");
  BOOST_CHECK_EQUAL(face->getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(face->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);

  face->setId(222);
  BOOST_CHECK_EQUAL(face->getId(), 222);

  face->setPersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
}

BOOST_AUTO_TEST_CASE(State)
{
  auto face = make_shared<DummyFace>();
  BOOST_CHECK_EQUAL(face->getState(), FaceState::UP);

  face->setState(FaceState::DOWN);
  BOOST_CHECK_EQUAL(face->getState(), FaceState::DOWN);
}

BOOST_AUTO_TEST_CASE(LinkServiceSendReceive)
{
  auto face1 = make_shared<DummyFace>();

  const size_t nInInterests = 192;
  const size_t nInData = 91;
  const size_t nInNacks = 29;
  const size_t nOutInterests = 202;
  const size_t nOutData = 128;
  const size_t nOutNacks = 84;

  size_t nReceivedInterests = 0;
  size_t nReceivedData = 0;
  size_t nReceivedNacks = 0;
  face1->afterReceiveInterest.connect(bind([&nReceivedInterests] { ++nReceivedInterests; }));
  face1->afterReceiveData.connect(bind([&nReceivedData] { ++nReceivedData; }));
  face1->afterReceiveNack.connect(bind([&nReceivedNacks] { ++nReceivedNacks; }));

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
    face1->sendInterest(*interest);
  }

  for (size_t i = 0; i < nOutData; ++i) {
    shared_ptr<Data> data = makeData("/GigPEtPH6");
    face1->sendData(*data);
  }

  for (size_t i = 0; i < nOutNacks; ++i) {
    lp::Nack nack = makeNack("/9xK6FbwIBM", 365, lp::NackReason::CONGESTION);
    face1->sendNack(nack);
  }

  BOOST_CHECK_EQUAL(face1->getCounters().nInInterests, nInInterests);
  BOOST_CHECK_EQUAL(face1->getCounters().nInData, nInData);
  BOOST_CHECK_EQUAL(face1->getCounters().nInNacks, nInNacks);
  BOOST_CHECK_EQUAL(face1->getCounters().nOutInterests, nOutInterests);
  BOOST_CHECK_EQUAL(face1->getCounters().nOutData, nOutData);
  BOOST_CHECK_EQUAL(face1->getCounters().nOutNacks, nOutNacks);

  BOOST_CHECK_EQUAL(nReceivedInterests, nInInterests);
  BOOST_CHECK_EQUAL(nReceivedData, nInData);
  BOOST_CHECK_EQUAL(nReceivedNacks, nInNacks);
  BOOST_CHECK_EQUAL(face1->sentInterests.size(), nOutInterests);
  BOOST_CHECK_EQUAL(face1->sentData.size(), nOutData);
  BOOST_CHECK_EQUAL(face1->sentNacks.size(), nOutNacks);
}

BOOST_AUTO_TEST_SUITE_END() // TestFace
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
