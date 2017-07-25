/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "table/pit-entry.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace pit {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestPitEntry, BaseFixture)

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(CanMatch, 1)
BOOST_AUTO_TEST_CASE(CanMatch)
{
  shared_ptr<Interest> interest0 = makeInterest("/A");
  Entry entry(*interest0);

  shared_ptr<Interest> interest1 = makeInterest("/B");
  BOOST_CHECK_EQUAL(entry.canMatch(*interest1), false);

  shared_ptr<Interest> interest2 = makeInterest("/A");
  interest2->setNonce(27956);
  BOOST_CHECK_EQUAL(entry.canMatch(*interest2), true);

  shared_ptr<Interest> interest3 = makeInterest("/A");
  interest3->setInterestLifetime(time::milliseconds(6210));
  BOOST_CHECK_EQUAL(entry.canMatch(*interest3), true);

  shared_ptr<Interest> interest4 = makeInterest("/A");
  interest4->setForwardingHint({{10, "/telia/terabits"}, {20, "/ucla/cs"}});
  BOOST_CHECK_EQUAL(entry.canMatch(*interest4), false); // expected failure until #3162

  shared_ptr<Interest> interest5 = makeInterest("/A");
  interest5->setMaxSuffixComponents(21);
  BOOST_CHECK_EQUAL(entry.canMatch(*interest5), false);
}

BOOST_AUTO_TEST_CASE(InOutRecords)
{
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  Name name("ndn:/KuYfjtRq");
  shared_ptr<Interest> interest  = makeInterest(name);
  shared_ptr<Interest> interest1 = makeInterest(name);
  interest1->setInterestLifetime(time::milliseconds(2528));
  interest1->setNonce(25559);
  shared_ptr<Interest> interest2 = makeInterest(name);
  interest2->setInterestLifetime(time::milliseconds(6464));
  interest2->setNonce(19004);
  shared_ptr<Interest> interest3 = makeInterest(name);
  interest3->setInterestLifetime(time::milliseconds(3585));
  interest3->setNonce(24216);
  shared_ptr<Interest> interest4 = makeInterest(name);
  interest4->setInterestLifetime(time::milliseconds(8795));
  interest4->setNonce(17365);

  Entry entry(*interest);

  BOOST_CHECK_EQUAL(entry.getInterest().getName(), name);
  BOOST_CHECK_EQUAL(entry.getName(), name);

  const InRecordCollection& inRecords1 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords1.size(), 0);
  const OutRecordCollection& outRecords1 = entry.getOutRecords();
  BOOST_CHECK_EQUAL(outRecords1.size(), 0);

  // insert in-record
  time::steady_clock::TimePoint before1 = time::steady_clock::now();
  InRecordCollection::iterator in1 = entry.insertOrUpdateInRecord(*face1, *interest1);
  time::steady_clock::TimePoint after1 = time::steady_clock::now();
  const InRecordCollection& inRecords2 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords2.size(), 1);
  BOOST_CHECK(in1 == inRecords2.begin());
  BOOST_CHECK_EQUAL(&in1->getFace(), face1.get());
  BOOST_CHECK_EQUAL(in1->getLastNonce(), interest1->getNonce());
  BOOST_CHECK_GE(in1->getLastRenewed(), before1);
  BOOST_CHECK_LE(in1->getLastRenewed(), after1);
  BOOST_CHECK_LE(in1->getExpiry() - in1->getLastRenewed()
                 - interest1->getInterestLifetime(),
                 (after1 - before1));
  BOOST_CHECK(in1 == entry.getInRecord(*face1));

  // insert out-record
  time::steady_clock::TimePoint before2 = time::steady_clock::now();
  OutRecordCollection::iterator out1 = entry.insertOrUpdateOutRecord(*face1, *interest1);
  time::steady_clock::TimePoint after2 = time::steady_clock::now();
  const OutRecordCollection& outRecords2 = entry.getOutRecords();
  BOOST_CHECK_EQUAL(outRecords2.size(), 1);
  BOOST_CHECK(out1 == outRecords2.begin());
  BOOST_CHECK_EQUAL(&out1->getFace(), face1.get());
  BOOST_CHECK_EQUAL(out1->getLastNonce(), interest1->getNonce());
  BOOST_CHECK_GE(out1->getLastRenewed(), before2);
  BOOST_CHECK_LE(out1->getLastRenewed(), after2);
  BOOST_CHECK_LE(out1->getExpiry() - out1->getLastRenewed()
                 - interest1->getInterestLifetime(),
                 (after2 - before2));
  BOOST_CHECK(out1 == entry.getOutRecord(*face1));

  // update in-record
  time::steady_clock::TimePoint before3 = time::steady_clock::now();
  InRecordCollection::iterator in2 = entry.insertOrUpdateInRecord(*face1, *interest2);
  time::steady_clock::TimePoint after3 = time::steady_clock::now();
  const InRecordCollection& inRecords3 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords3.size(), 1);
  BOOST_CHECK(in2 == inRecords3.begin());
  BOOST_CHECK_EQUAL(&in2->getFace(), face1.get());
  BOOST_CHECK_EQUAL(in2->getLastNonce(), interest2->getNonce());
  BOOST_CHECK_LE(in2->getExpiry() - in2->getLastRenewed()
                 - interest2->getInterestLifetime(),
                 (after3 - before3));

  // insert another in-record
  InRecordCollection::iterator in3 = entry.insertOrUpdateInRecord(*face2, *interest3);
  const InRecordCollection& inRecords4 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords4.size(), 2);
  BOOST_CHECK_EQUAL(&in3->getFace(), face2.get());

  // get in-record
  InRecordCollection::iterator in4 = entry.getInRecord(*face1);
  BOOST_REQUIRE(in4 != entry.in_end());
  BOOST_CHECK_EQUAL(&in4->getFace(), face1.get());

  // clear in-records
  entry.clearInRecords();
  const InRecordCollection& inRecords5 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords5.size(), 0);
  BOOST_CHECK(entry.getInRecord(*face1) == entry.in_end());

  // insert another out-record
  OutRecordCollection::iterator out2 =
    entry.insertOrUpdateOutRecord(*face2, *interest4);
  const OutRecordCollection& outRecords3 = entry.getOutRecords();
  BOOST_CHECK_EQUAL(outRecords3.size(), 2);
  BOOST_CHECK_EQUAL(&out2->getFace(), face2.get());

  // get out-record
  OutRecordCollection::iterator out3 = entry.getOutRecord(*face1);
  BOOST_REQUIRE(out3 != entry.out_end());
  BOOST_CHECK_EQUAL(&out3->getFace(), face1.get());

  // delete out-record
  entry.deleteOutRecord(*face2);
  const OutRecordCollection& outRecords4 = entry.getOutRecords();
  BOOST_REQUIRE_EQUAL(outRecords4.size(), 1);
  BOOST_CHECK_EQUAL(&outRecords4.begin()->getFace(), face1.get());
  BOOST_CHECK(entry.getOutRecord(*face2) == entry.out_end());
}

BOOST_AUTO_TEST_CASE(Lifetime)
{
  shared_ptr<Interest> interest = makeInterest("ndn:/7oIEurbgy6");

  shared_ptr<Face> face = make_shared<DummyFace>();
  Entry entry(*interest);

  InRecordCollection::iterator inIt = entry.insertOrUpdateInRecord(*face, *interest);
  BOOST_CHECK_GT(inIt->getExpiry(), time::steady_clock::now());

  OutRecordCollection::iterator outIt = entry.insertOrUpdateOutRecord(*face, *interest);
  BOOST_CHECK_GT(outIt->getExpiry(), time::steady_clock::now());
}

BOOST_AUTO_TEST_CASE(OutRecordNack)
{
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  OutRecord outR(*face1);
  BOOST_CHECK(outR.getIncomingNack() == nullptr);

  shared_ptr<Interest> interest1 = makeInterest("ndn:/uWiapGjYL");
  interest1->setNonce(165);
  outR.update(*interest1);
  BOOST_CHECK(outR.getIncomingNack() == nullptr);

  shared_ptr<Interest> interest2 = makeInterest("ndn:/uWiapGjYL");
  interest2->setNonce(996);
  lp::Nack nack2(*interest2);
  nack2.setReason(lp::NackReason::CONGESTION);
  BOOST_CHECK_EQUAL(outR.setIncomingNack(nack2), false);
  BOOST_CHECK(outR.getIncomingNack() == nullptr);

  lp::Nack nack1(*interest1);
  nack1.setReason(lp::NackReason::DUPLICATE);
  BOOST_CHECK_EQUAL(outR.setIncomingNack(nack1), true);
  BOOST_REQUIRE(outR.getIncomingNack() != nullptr);
  BOOST_CHECK_EQUAL(outR.getIncomingNack()->getReason(), lp::NackReason::DUPLICATE);

  outR.clearIncomingNack();
  BOOST_CHECK(outR.getIncomingNack() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END() // TestPitEntry
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace pit
} // namespace nfd
