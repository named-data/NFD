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

#include "table/cleanup.hpp"
#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestCleanup, BaseFixture)

BOOST_AUTO_TEST_SUITE(FaceRemoval)

BOOST_AUTO_TEST_CASE(Basic)
{
  NameTree nameTree(16);
  Fib fib(nameTree);
  Pit pit(nameTree);
  shared_ptr<Face> face1 = make_shared<DummyFace>();

  for (uint64_t i = 0; i < 300; ++i) {
    Name name = Name("/P").appendVersion(i);

    fib::Entry* fibEntry = fib.insert(name).first;
    fibEntry->addNextHop(*face1, 0);

    shared_ptr<Interest> interest = makeInterest(name);
    shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;
    if ((i & 0x01) != 0) {
      pitEntry->insertOrUpdateInRecord(*face1, *interest);
    }
    if ((i & 0x02) != 0) {
      pitEntry->insertOrUpdateOutRecord(*face1, *interest);
    }
  }
  BOOST_CHECK_EQUAL(fib.size(), 300);
  BOOST_CHECK_EQUAL(pit.size(), 300);

  cleanupOnFaceRemoval(nameTree, fib, pit, *face1);
  BOOST_CHECK_EQUAL(fib.size(), 0);
  BOOST_CHECK_EQUAL(pit.size(), 300);
  for (const pit::Entry& pitEntry : pit) {
    BOOST_CHECK_EQUAL(pitEntry.hasInRecords(), false);
    BOOST_CHECK_EQUAL(pitEntry.hasOutRecords(), false);
  }
}

BOOST_AUTO_TEST_CASE(RemoveFibNexthops)
{
  Forwarder forwarder;
  NameTree& nameTree = forwarder.getNameTree();
  Fib& fib = forwarder.getFib();

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  // {}
  size_t nNameTreeEntriesBefore = nameTree.size();
  BOOST_CHECK_EQUAL(fib.size(), 0);

  fib::Entry* entryA = fib.insert("/A").first;
  entryA->addNextHop(*face1, 0);
  entryA->addNextHop(*face2, 0);
  // {'/A':[1,2]}

  fib::Entry* entryB = fib.insert("/B").first;
  entryB->addNextHop(*face1, 0);
  // {'/A':[1,2], '/B':[1]}

  fib::Entry* entryC = fib.insert("/C").first;
  entryC->addNextHop(*face2, 1);
  // {'/A':[1,2], '/B':[1], '/C':[2]}

  fib::Entry* entryB1 = fib.insert("/B/1").first;
  entryB1->addNextHop(*face1, 0);
  // {'/A':[1,2], '/B':[1], '/B/1':[1], '/C':[2]}

  fib::Entry* entryB12 = fib.insert("/B/1/2").first;
  entryB12->addNextHop(*face1, 0);
  // {'/A':[1,2], '/B':[1], '/B/1':[1], '/B/1/2':[1], '/C':[2]}

  // ---- close face1 ----
  face1->close();
  BOOST_CHECK_EQUAL(face1->getState(), face::FaceState::CLOSED);
  // {'/A':[2], '/C':[2]}
  BOOST_CHECK_EQUAL(fib.size(), 2);

  const fib::Entry& foundA = fib.findLongestPrefixMatch("/A");
  BOOST_CHECK_EQUAL(foundA.getPrefix(), "/A");
  BOOST_CHECK_EQUAL(foundA.getNextHops().size(), 1);
  BOOST_CHECK_EQUAL(&foundA.getNextHops().begin()->getFace(), face2.get());

  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/B").getPrefix(), "/");

  // ---- close face2 ----
  face2->close();
  BOOST_CHECK_EQUAL(face2->getState(), face::FaceState::CLOSED);
  BOOST_CHECK_EQUAL(fib.size(), 0);
  BOOST_CHECK_EQUAL(nameTree.size(), nNameTreeEntriesBefore);
}

BOOST_AUTO_TEST_CASE(DeletePitInOutRecords)
{
  Forwarder forwarder;
  Pit& pit = forwarder.getPit();

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  // {}
  BOOST_CHECK_EQUAL(pit.size(), 0);

  shared_ptr<Interest> interestA = makeInterest("/A");
  shared_ptr<pit::Entry> entryA = pit.insert(*interestA).first;
  entryA->insertOrUpdateInRecord(*face1, *interestA);
  entryA->insertOrUpdateInRecord(*face2, *interestA);
  entryA->insertOrUpdateOutRecord(*face1, *interestA);
  entryA->insertOrUpdateOutRecord(*face2, *interestA);
  // {'/A':[1,2]}

  shared_ptr<Interest> interestB = makeInterest("/B");
  shared_ptr<pit::Entry> entryB = pit.insert(*interestB).first;
  entryB->insertOrUpdateInRecord(*face1, *interestB);
  entryB->insertOrUpdateOutRecord(*face1, *interestB);
  // {'/A':[1,2], '/B':[1]}

  shared_ptr<Interest> interestC = makeInterest("/C");
  shared_ptr<pit::Entry> entryC = pit.insert(*interestC).first;
  entryC->insertOrUpdateInRecord(*face2, *interestC);
  entryC->insertOrUpdateOutRecord(*face2, *interestC);
  // {'/A':[1,2], '/B':[1], '/C':[2]}
  BOOST_CHECK_EQUAL(pit.size(), 3);

  // ---- close face1 ----
  face1->close();
  BOOST_CHECK_EQUAL(face1->getState(), face::FaceState::CLOSED);
  // {'/A':[2], '/B':[], '/C':[2]}
  BOOST_CHECK_EQUAL(pit.size(), 3);

  shared_ptr<pit::Entry> foundA = pit.find(*interestA);
  BOOST_REQUIRE(foundA != nullptr);
  BOOST_REQUIRE_EQUAL(foundA->getInRecords().size(), 1);
  BOOST_CHECK_EQUAL(&foundA->getInRecords().front().getFace(), face2.get());
  BOOST_REQUIRE_EQUAL(foundA->getOutRecords().size(), 1);
  BOOST_CHECK_EQUAL(&foundA->getOutRecords().front().getFace(), face2.get());
}

BOOST_AUTO_TEST_SUITE_END() // FaceRemovalCleanup

BOOST_AUTO_TEST_SUITE_END() // TestCleanup
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace nfd
