/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "choose-strategy.hpp"
#include "dummy-strategy.hpp"

#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestPitExpiry, GlobalIoTimeFixture)

class PitExpiryTestStrategy : public DummyStrategy
{
public:
  static Name
  getStrategyName(uint64_t version)
  {
    return Name("/PitExpiryTestStrategy").appendVersion(version);
  }

  static void
  registerAs(const Name& strategyName)
  {
    registerAsImpl<PitExpiryTestStrategy>(strategyName);
  }

  explicit
  PitExpiryTestStrategy(Forwarder& forwarder, const Name& name = getStrategyName(1))
    : DummyStrategy(forwarder, name)
  {
  }

  void
  afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override
  {
    DummyStrategy::afterReceiveInterest(ingress, interest, pitEntry);

    if (afterReceiveInterest_count <= 1) {
      setExpiryTimer(pitEntry, 190_ms);
    }
  }

  void
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const FaceEndpoint& ingress, const Data& data) override
  {
    DummyStrategy::beforeSatisfyInterest(pitEntry, ingress, data);

    if (beforeSatisfyInterest_count <= 2) {
      setExpiryTimer(pitEntry, 190_ms);
    }
  }

  void
  afterContentStoreHit(const shared_ptr<pit::Entry>& pitEntry,
                       const FaceEndpoint& ingress, const Data& data) override
  {
    if (afterContentStoreHit_count == 0) {
      setExpiryTimer(pitEntry, 190_ms);
    }

    DummyStrategy::afterContentStoreHit(pitEntry, ingress, data);
  }

  void
  afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,
                   const FaceEndpoint& ingress, const Data& data) override
  {
    ++afterReceiveData_count;

    if (afterReceiveData_count <= 2) {
      setExpiryTimer(pitEntry, 290_ms);
    }

    this->sendDataToAll(pitEntry, ingress, data);
  }

  void
  afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry) override
  {
    DummyStrategy::afterReceiveNack(ingress, nack, pitEntry);

    if (afterReceiveNack_count <= 1) {
      setExpiryTimer(pitEntry, 50_ms);
    }
  }
};

BOOST_AUTO_TEST_CASE(UnsatisfiedInterest)
{
  FaceTable faceTable;
  Forwarder forwarder(faceTable);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  faceTable.add(face1);
  faceTable.add(face2);

  Pit& pit = forwarder.getPit();

  auto interest1 = makeInterest("/A/0");
  auto interest2 = makeInterest("/A/1");
  interest1->setInterestLifetime(90_ms);
  interest2->setInterestLifetime(90_ms);

  face1->receiveInterest(*interest1, 0);
  face2->receiveInterest(*interest2, 0);
  BOOST_CHECK_EQUAL(pit.size(), 2);

  this->advanceClocks(100_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_CASE(SatisfiedInterest)
{
  FaceTable faceTable;
  Forwarder forwarder(faceTable);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  faceTable.add(face1);
  faceTable.add(face2);

  Pit& pit = forwarder.getPit();

  auto interest = makeInterest("/A/0");
  interest->setInterestLifetime(90_ms);
  auto data = makeData("/A/0");

  face1->receiveInterest(*interest, 0);

  this->advanceClocks(30_ms);
  face2->receiveData(*data, 0);

  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_CASE(CsHit)
{
  FaceTable faceTable;
  Forwarder forwarder(faceTable);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  faceTable.add(face1);
  faceTable.add(face2);

  Name strategyA("/strategyA/%FD%01");
  PitExpiryTestStrategy::registerAs(strategyA);
  choose<PitExpiryTestStrategy>(forwarder, "/A", strategyA);

  auto interest = makeInterest("/A/0");
  interest->setInterestLifetime(90_ms);

  auto data = makeData("/A/0");
  data->setTag(make_shared<lp::IncomingFaceIdTag>(face2->getId()));

  Pit& pit = forwarder.getPit();
  BOOST_CHECK_EQUAL(pit.size(), 0);

  Cs& cs = forwarder.getCs();
  cs.insert(*data);

  face1->receiveInterest(*interest, 0);
  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  this->advanceClocks(190_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);

  face1->receiveInterest(*interest, 0);
  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_CASE(ReceiveNack)
{
  FaceTable faceTable;
  Forwarder forwarder(faceTable);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>();
  faceTable.add(face1);
  faceTable.add(face2);
  faceTable.add(face3);

  Name strategyA("/strategyA/%FD%01");
  PitExpiryTestStrategy::registerAs(strategyA);
  choose<PitExpiryTestStrategy>(forwarder, "/A", strategyA);

  Pit& pit = forwarder.getPit();

  auto interest = makeInterest("/A/0", false, 90_ms, 562);
  lp::Nack nack = makeNack(*interest, lp::NackReason::CONGESTION);

  face1->receiveInterest(*interest, 0);
  auto entry = pit.find(*interest);
  entry->insertOrUpdateOutRecord(*face2, *interest);
  entry->insertOrUpdateOutRecord(*face3, *interest);

  this->advanceClocks(10_ms);
  face2->receiveNack(nack, 0);

  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  this->advanceClocks(50_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_CASE(ResetTimerAfterReceiveInterest)
{
  FaceTable faceTable;
  Forwarder forwarder(faceTable);

  auto face = make_shared<DummyFace>();
  faceTable.add(face);

  Name strategyA("/strategyA/%FD%01");
  PitExpiryTestStrategy::registerAs(strategyA);
  choose<PitExpiryTestStrategy>(forwarder, "/A", strategyA);

  Pit& pit = forwarder.getPit();

  auto interest = makeInterest("/A/0", false, 90_ms);

  face->receiveInterest(*interest, 0);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  this->advanceClocks(100_ms);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  this->advanceClocks(100_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_CASE(ResetTimerBeforeSatisfyInterest)
{
  FaceTable faceTable;
  Forwarder forwarder(faceTable);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>();
  faceTable.add(face1);
  faceTable.add(face2);
  faceTable.add(face3);

  Name strategyA("/strategyA/%FD%01");
  Name strategyB("/strategyB/%FD%01");
  PitExpiryTestStrategy::registerAs(strategyA);
  PitExpiryTestStrategy::registerAs(strategyB);
  auto& sA = choose<PitExpiryTestStrategy>(forwarder, "/A", strategyA);
  auto& sB = choose<PitExpiryTestStrategy>(forwarder, "/A/0", strategyB);
  Pit& pit = forwarder.getPit();

  auto interest1 = makeInterest("/A", true, 90_ms);
  auto interest2 = makeInterest("/A/0", false, 90_ms);
  auto data = makeData("/A/0");

  face1->receiveInterest(*interest1, 0);
  face2->receiveInterest(*interest2, 0);
  BOOST_CHECK_EQUAL(pit.size(), 2);

  // beforeSatisfyInterest: the first Data prolongs PIT expiry timer by 190 ms
  this->advanceClocks(30_ms);
  face3->receiveData(*data, 0);
  this->advanceClocks(189_ms);
  BOOST_CHECK_EQUAL(pit.size(), 2);
  this->advanceClocks(2_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);

  face1->receiveInterest(*interest1, 0);
  face2->receiveInterest(*interest2, 0);

  // beforeSatisfyInterest: the second Data prolongs PIT expiry timer
  // and the third one sets the timer to now
  this->advanceClocks(30_ms);
  face3->receiveData(*data, 0);
  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 2);

  this->advanceClocks(30_ms);
  face3->receiveData(*data, 0);
  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);

  BOOST_CHECK_EQUAL(sA.beforeSatisfyInterest_count, 3);
  BOOST_CHECK_EQUAL(sB.beforeSatisfyInterest_count, 3);
  BOOST_CHECK_EQUAL(sA.afterReceiveData_count, 0);
  BOOST_CHECK_EQUAL(sB.afterReceiveData_count, 0);
}

BOOST_AUTO_TEST_CASE(ResetTimerAfterReceiveData)
{
  FaceTable faceTable;
  Forwarder forwarder(faceTable);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  faceTable.add(face1);
  faceTable.add(face2);

  Name strategyA("/strategyA/%FD%01");
  PitExpiryTestStrategy::registerAs(strategyA);
  auto& sA = choose<PitExpiryTestStrategy>(forwarder, "/A", strategyA);

  Pit& pit = forwarder.getPit();

  auto interest = makeInterest("/A/0", false, 90_ms);
  auto data = makeData("/A/0");

  face1->receiveInterest(*interest, 0);

  // afterReceiveData: the first Data prolongs PIT expiry timer by 290 ms
  this->advanceClocks(30_ms);
  face2->receiveData(*data, 0);
  this->advanceClocks(289_ms);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  this->advanceClocks(2_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);

  face1->receiveInterest(*interest, 0);

  // afterReceiveData: the second Data prolongs PIT expiry timer
  // and the third one sets the timer to now
  this->advanceClocks(30_ms);
  face2->receiveData(*data, 0);
  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  this->advanceClocks(30_ms);
  face2->receiveData(*data, 0);
  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);

  BOOST_CHECK_EQUAL(sA.beforeSatisfyInterest_count, 0);
  BOOST_CHECK_EQUAL(sA.afterReceiveData_count, 3);
}

BOOST_AUTO_TEST_CASE(ReceiveNackAfterResetTimer)
{
  FaceTable faceTable;
  Forwarder forwarder(faceTable);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>();
  faceTable.add(face1);
  faceTable.add(face2);
  faceTable.add(face3);

  Name strategyA("/strategyA/%FD%01");
  PitExpiryTestStrategy::registerAs(strategyA);
  choose<PitExpiryTestStrategy>(forwarder, "/A", strategyA);

  Pit& pit = forwarder.getPit();

  auto interest = makeInterest("/A/0", false, 90_ms, 562);
  lp::Nack nack = makeNack(*interest, lp::NackReason::CONGESTION);

  face1->receiveInterest(*interest, 0);
  auto entry = pit.find(*interest);
  entry->insertOrUpdateOutRecord(*face2, *interest);
  entry->insertOrUpdateOutRecord(*face3, *interest);

  //pitEntry is not erased after receiving the first Nack
  this->advanceClocks(10_ms);
  face2->receiveNack(nack, 0);
  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  //pitEntry is erased after receiving the second Nack
  this->advanceClocks(10_ms);
  face3->receiveNack(nack, 0);
  this->advanceClocks(1_ms);
  BOOST_CHECK_EQUAL(pit.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestPitExpiry
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
