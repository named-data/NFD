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

#include "fw/algorithm.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestAlgorithm, BaseFixture)

class ScopeControlFixture : public BaseFixture
{
protected:
  ScopeControlFixture()
    : nonLocalFace1(make_shared<DummyFace>("dummy://1", "dummy://1", ndn::nfd::FACE_SCOPE_NON_LOCAL))
    , nonLocalFace2(make_shared<DummyFace>("dummy://2", "dummy://2", ndn::nfd::FACE_SCOPE_NON_LOCAL))
    , localFace3(make_shared<DummyFace>("dummy://3", "dummy://3", ndn::nfd::FACE_SCOPE_LOCAL))
    , localFace4(make_shared<DummyFace>("dummy://4", "dummy://4", ndn::nfd::FACE_SCOPE_LOCAL))
  {
  }

protected:
  shared_ptr<Face> nonLocalFace1;
  shared_ptr<Face> nonLocalFace2;
  shared_ptr<Face> localFace3;
  shared_ptr<Face> localFace4;
};

BOOST_FIXTURE_TEST_SUITE(WouldViolateScope, ScopeControlFixture)

BOOST_AUTO_TEST_CASE(Unrestricted)
{
  shared_ptr<Interest> interest = makeInterest("ndn:/ieWRzDsCu");

  BOOST_CHECK_EQUAL(wouldViolateScope(*nonLocalFace1, *interest, *nonLocalFace2), false);
  BOOST_CHECK_EQUAL(wouldViolateScope(*nonLocalFace1, *interest, *localFace4), false);
  BOOST_CHECK_EQUAL(wouldViolateScope(*localFace3, *interest, *nonLocalFace2), false);
  BOOST_CHECK_EQUAL(wouldViolateScope(*localFace3, *interest, *localFace4), false);
}

BOOST_AUTO_TEST_CASE(Localhost)
{
  shared_ptr<Interest> interest = makeInterest("ndn:/localhost/5n1LzIt3");

  // /localhost Interests from non-local faces should be rejected by incoming Interest pipeline,
  // and are not tested here.
  BOOST_CHECK_EQUAL(wouldViolateScope(*localFace3, *interest, *nonLocalFace2), true);
  BOOST_CHECK_EQUAL(wouldViolateScope(*localFace3, *interest, *localFace4), false);
}

BOOST_AUTO_TEST_CASE(Localhop)
{
  shared_ptr<Interest> interest = makeInterest("ndn:/localhop/YcIKWCRYJ");

  BOOST_CHECK_EQUAL(wouldViolateScope(*nonLocalFace1, *interest, *nonLocalFace2), true);
  BOOST_CHECK_EQUAL(wouldViolateScope(*nonLocalFace1, *interest, *localFace4), false);
  BOOST_CHECK_EQUAL(wouldViolateScope(*localFace3, *interest, *nonLocalFace2), false);
  BOOST_CHECK_EQUAL(wouldViolateScope(*localFace3, *interest, *localFace4), false);
}

BOOST_AUTO_TEST_SUITE_END() // WouldViolateScope

BOOST_AUTO_TEST_CASE(CanForwardToLegacy)
{
  shared_ptr<Interest> interest = makeInterest("ndn:/WDsuBLIMG");
  pit::Entry entry(*interest);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();

  entry.insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK_EQUAL(canForwardToLegacy(entry, *face1), false);
  BOOST_CHECK_EQUAL(canForwardToLegacy(entry, *face2), true);

  entry.insertOrUpdateInRecord(*face2, *interest);
  BOOST_CHECK_EQUAL(canForwardToLegacy(entry, *face1), true);
  BOOST_CHECK_EQUAL(canForwardToLegacy(entry, *face2), true);

  entry.insertOrUpdateOutRecord(*face1, *interest);
  BOOST_CHECK_EQUAL(canForwardToLegacy(entry, *face1), false);
  BOOST_CHECK_EQUAL(canForwardToLegacy(entry, *face2), true);
}

BOOST_AUTO_TEST_CASE(Nonce)
{
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();

  shared_ptr<Interest> interest = makeInterest("ndn:/qtCQ7I1c");
  interest->setNonce(25559);

  pit::Entry entry0(*interest);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry0, 25559, *face1), DUPLICATE_NONCE_NONE);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry0, 25559, *face2), DUPLICATE_NONCE_NONE);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry0, 19004, *face1), DUPLICATE_NONCE_NONE);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry0, 19004, *face2), DUPLICATE_NONCE_NONE);

  pit::Entry entry1(*interest);
  entry1.insertOrUpdateInRecord(*face1, *interest);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry1, 25559, *face1), DUPLICATE_NONCE_IN_SAME);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry1, 25559, *face2), DUPLICATE_NONCE_IN_OTHER);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry1, 19004, *face1), DUPLICATE_NONCE_NONE);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry1, 19004, *face2), DUPLICATE_NONCE_NONE);

  pit::Entry entry2(*interest);
  entry2.insertOrUpdateOutRecord(*face1, *interest);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry2, 25559, *face1), DUPLICATE_NONCE_OUT_SAME);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry2, 25559, *face2), DUPLICATE_NONCE_OUT_OTHER);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry2, 19004, *face1), DUPLICATE_NONCE_NONE);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry2, 19004, *face2), DUPLICATE_NONCE_NONE);

  pit::Entry entry3(*interest);
  entry3.insertOrUpdateInRecord(*face1, *interest);
  entry3.insertOrUpdateOutRecord(*face1, *interest);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry3, 25559, *face1),
                    DUPLICATE_NONCE_IN_SAME | DUPLICATE_NONCE_OUT_SAME);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry3, 25559, *face2),
                    DUPLICATE_NONCE_IN_OTHER | DUPLICATE_NONCE_OUT_OTHER);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry3, 19004, *face1), DUPLICATE_NONCE_NONE);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry3, 19004, *face2), DUPLICATE_NONCE_NONE);

  pit::Entry entry4(*interest);
  entry4.insertOrUpdateInRecord(*face1, *interest);
  entry4.insertOrUpdateInRecord(*face2, *interest);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry4, 25559, *face1),
                    DUPLICATE_NONCE_IN_SAME | DUPLICATE_NONCE_IN_OTHER);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry4, 25559, *face2),
                    DUPLICATE_NONCE_IN_SAME | DUPLICATE_NONCE_IN_OTHER);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry4, 19004, *face1), DUPLICATE_NONCE_NONE);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry4, 19004, *face2), DUPLICATE_NONCE_NONE);

  pit::Entry entry5(*interest);
  entry5.insertOrUpdateOutRecord(*face1, *interest);
  entry5.insertOrUpdateOutRecord(*face2, *interest);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry5, 25559, *face1),
                    DUPLICATE_NONCE_OUT_SAME | DUPLICATE_NONCE_OUT_OTHER);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry5, 25559, *face2),
                    DUPLICATE_NONCE_OUT_SAME | DUPLICATE_NONCE_OUT_OTHER);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry5, 19004, *face1), DUPLICATE_NONCE_NONE);
  BOOST_CHECK_EQUAL(findDuplicateNonce(entry5, 19004, *face2), DUPLICATE_NONCE_NONE);
}

BOOST_FIXTURE_TEST_CASE(HasPendingOutRecords, UnitTestTimeFixture)
{
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();
  auto face3 = make_shared<DummyFace>();

  shared_ptr<Interest> interest = makeInterest("ndn:/totzXG0d");
  interest->setNonce(29321);

  pit::Entry entry(*interest);
  BOOST_CHECK_EQUAL(hasPendingOutRecords(entry), false);

  // Interest-Data
  entry.insertOrUpdateOutRecord(*face1, *interest);
  BOOST_CHECK_EQUAL(hasPendingOutRecords(entry), true);
  entry.deleteOutRecord(*face1);
  BOOST_CHECK_EQUAL(hasPendingOutRecords(entry), false);

  // Interest-Nack
  entry.insertOrUpdateOutRecord(*face2, *interest);
  BOOST_CHECK_EQUAL(hasPendingOutRecords(entry), true);
  pit::OutRecordCollection::iterator outR = entry.getOutRecord(*face2);
  BOOST_REQUIRE(outR != entry.out_end());
  lp::Nack nack = makeNack("ndn:/totzXG0d", 29321, lp::NackReason::DUPLICATE);
  bool isNackAccepted = outR->setIncomingNack(nack); // Nack arrival
  BOOST_REQUIRE(isNackAccepted);
  BOOST_CHECK_EQUAL(hasPendingOutRecords(entry), false);

  // Interest-timeout
  entry.insertOrUpdateOutRecord(*face3, *interest);
  BOOST_CHECK_EQUAL(hasPendingOutRecords(entry), true);
  this->advanceClocks(ndn::DEFAULT_INTEREST_LIFETIME, 2);
  BOOST_CHECK_EQUAL(hasPendingOutRecords(entry), false);
}

BOOST_FIXTURE_TEST_CASE(GetLastOutgoing, UnitTestTimeFixture)
{
  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>();

  shared_ptr<Interest> interest = makeInterest("ndn:/c1I7QCtc");
  pit::Entry entry(*interest);

  time::steady_clock::TimePoint before = time::steady_clock::now();

  entry.insertOrUpdateOutRecord(*face1, *interest);
  this->advanceClocks(time::milliseconds(1000));

  BOOST_CHECK_EQUAL(getLastOutgoing(entry), before);

  entry.insertOrUpdateOutRecord(*face2, *interest);

  BOOST_CHECK_EQUAL(getLastOutgoing(entry), time::steady_clock::now());
}

BOOST_AUTO_TEST_SUITE_END() // TestPitAlgorithm
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
