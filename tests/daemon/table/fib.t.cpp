/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#include "table/fib.hpp"
#include "table/pit.hpp"
#include "table/measurements.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include <ndn-cxx/util/concepts.hpp>

namespace nfd::tests {

using namespace nfd::fib;

NDN_CXX_ASSERT_FORWARD_ITERATOR(Fib::const_iterator);

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestFib, GlobalIoFixture)

BOOST_AUTO_TEST_CASE(FibEntry)
{
  NameTree nameTree;
  Fib fib(nameTree);

  Name prefix("/pxWhfFza");
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();

  Face* expectedFace = nullptr;
  uint64_t expectedCost = 0;
  size_t nNewNextHopSignals = 0;

  fib.afterNewNextHop.connect(
    [&] (const Name& prefix1, const NextHop& nextHop) {
      ++nNewNextHopSignals;
      BOOST_CHECK_EQUAL(prefix1, prefix);
      BOOST_CHECK_EQUAL(&nextHop.getFace(), expectedFace);
      BOOST_CHECK_EQUAL(nextHop.getCost(), expectedCost);
    });

  Entry& entry = *fib.insert(prefix).first;

  BOOST_CHECK_EQUAL(entry.getPrefix(), prefix);

  // []
  BOOST_CHECK_EQUAL(entry.getNextHops().size(), 0);

  expectedFace = face1.get();
  expectedCost = 20;

  fib.addOrUpdateNextHop(entry, *face1, 20);
  // [(face1,20)]
  BOOST_CHECK_EQUAL(entry.getNextHops().size(), 1);
  BOOST_CHECK_EQUAL(&entry.getNextHops().begin()->getFace(), face1.get());
  BOOST_CHECK_EQUAL(entry.getNextHops().begin()->getCost(), 20);
  BOOST_CHECK_EQUAL(nNewNextHopSignals, 1);

  fib.addOrUpdateNextHop(entry, *face1, 30);
  // [(face1,30)]
  BOOST_CHECK_EQUAL(entry.getNextHops().size(), 1);
  BOOST_CHECK_EQUAL(&entry.getNextHops().begin()->getFace(), face1.get());
  BOOST_CHECK_EQUAL(entry.getNextHops().begin()->getCost(), 30);
  BOOST_CHECK_EQUAL(nNewNextHopSignals, 1);

  expectedFace = face2.get();
  expectedCost = 40;

  fib.addOrUpdateNextHop(entry, *face2, 40);
  // [(face1,30), (face2,40)]
  BOOST_CHECK_EQUAL(entry.getNextHops().size(), 2);
  {
    auto it = entry.getNextHops().begin();
    BOOST_REQUIRE(it != entry.getNextHops().end());
    BOOST_CHECK_EQUAL(&it->getFace(), face1.get());
    BOOST_CHECK_EQUAL(it->getCost(), 30);

    ++it;
    BOOST_REQUIRE(it != entry.getNextHops().end());
    BOOST_CHECK_EQUAL(&it->getFace(), face2.get());
    BOOST_CHECK_EQUAL(it->getCost(), 40);

    ++it;
    BOOST_CHECK(it == entry.getNextHops().end());

    BOOST_CHECK_EQUAL(nNewNextHopSignals, 2);
  }

  fib.addOrUpdateNextHop(entry, *face2, 10);
  // [(face2,10), (face1,30)]
  BOOST_CHECK_EQUAL(entry.getNextHops().size(), 2);
  {
    auto it = entry.getNextHops().begin();
    BOOST_REQUIRE(it != entry.getNextHops().end());
    BOOST_CHECK_EQUAL(&it->getFace(), face2.get());
    BOOST_CHECK_EQUAL(it->getCost(), 10);

    ++it;
    BOOST_REQUIRE(it != entry.getNextHops().end());
    BOOST_CHECK_EQUAL(&it->getFace(), face1.get());
    BOOST_CHECK_EQUAL(it->getCost(), 30);

    ++it;
    BOOST_CHECK(it == entry.getNextHops().end());

    BOOST_CHECK_EQUAL(nNewNextHopSignals, 2);
  }

  Fib::RemoveNextHopResult status = fib.removeNextHop(entry, *face1);
  // [(face2,10)]
  BOOST_CHECK(status == Fib::RemoveNextHopResult::NEXTHOP_REMOVED);
  BOOST_CHECK_EQUAL(entry.getNextHops().size(), 1);
  BOOST_CHECK_EQUAL(entry.getNextHops().begin()->getFace().getId(), face2->getId());
  BOOST_CHECK_EQUAL(entry.getNextHops().begin()->getCost(), 10);

  status = fib.removeNextHop(entry, *face1);
  // [(face2,10)]
  BOOST_CHECK(status == Fib::RemoveNextHopResult::NO_SUCH_NEXTHOP);
  BOOST_CHECK_EQUAL(entry.getNextHops().size(), 1);
  BOOST_CHECK_EQUAL(entry.getNextHops().begin()->getFace().getId(), face2->getId());
  BOOST_CHECK_EQUAL(entry.getNextHops().begin()->getCost(), 10);

  status = fib.removeNextHop(entry, *face2);
  // []
  BOOST_CHECK(status == Fib::RemoveNextHopResult::FIB_ENTRY_REMOVED);
  BOOST_CHECK(fib.findExactMatch(prefix) == nullptr);
}

BOOST_AUTO_TEST_CASE(Insert_LongestPrefixMatch)
{
  NameTree nameTree;
  Fib fib(nameTree);

  // []
  BOOST_CHECK_EQUAL(fib.size(), 0);
  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/A").getPrefix(), "/"); // the empty entry

  std::pair<Entry*, bool> insertRes = fib.insert("/");
  BOOST_CHECK_EQUAL(insertRes.second, true);
  BOOST_REQUIRE(insertRes.first != nullptr);
  BOOST_CHECK_EQUAL(insertRes.first->getPrefix(), "/");
  // ['/']
  BOOST_CHECK_EQUAL(fib.size(), 1);

  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/A").getPrefix(), "/");

  insertRes = fib.insert("/A");
  BOOST_CHECK_EQUAL(insertRes.second, true);
  BOOST_REQUIRE(insertRes.first != nullptr);
  BOOST_CHECK_EQUAL(insertRes.first->getPrefix(), "/A");
  // ['/', '/A']
  BOOST_CHECK_EQUAL(fib.size(), 2);

  insertRes = fib.insert("/A");
  BOOST_CHECK_EQUAL(insertRes.second, false);
  BOOST_REQUIRE(insertRes.first != nullptr);
  BOOST_CHECK_EQUAL(insertRes.first->getPrefix(), "/A");
  // ['/', '/A']
  BOOST_CHECK_EQUAL(fib.size(), 2);

  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/A").getPrefix(), "/A");

  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/A/B/C/D").getPrefix(), "/A");

  insertRes = fib.insert("/A/B/C");
  BOOST_CHECK_EQUAL(insertRes.second, true);
  BOOST_REQUIRE(insertRes.first != nullptr);
  BOOST_CHECK_EQUAL(insertRes.first->getPrefix(), "/A/B/C");
  // ['/', '/A', '/A/B/C']
  BOOST_CHECK_EQUAL(fib.size(), 3);

  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/A").getPrefix(), "/A");
  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/A/B").getPrefix(), "/A");
  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/A/B/C/D").getPrefix(), "/A/B/C");
  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch("/E").getPrefix(), "/");
}

BOOST_AUTO_TEST_CASE(LongestPrefixMatchWithPitEntry)
{
  NameTree nameTree;
  Fib fib(nameTree);

  shared_ptr<Data> dataABC = makeData("/A/B/C");
  Name fullNameABC = dataABC->getFullName();
  shared_ptr<Data> dataADE = makeData("/A/D/E");
  Name fullNameADE = dataADE->getFullName();
  fib.insert("/A");
  fib.insert(fullNameABC);

  Pit pit(nameTree);
  shared_ptr<Interest> interestAB = makeInterest("/A/B");
  shared_ptr<pit::Entry> pitAB = pit.insert(*interestAB).first;
  shared_ptr<Interest> interestABC = makeInterest(fullNameABC);
  shared_ptr<pit::Entry> pitABC = pit.insert(*interestABC).first;
  shared_ptr<Interest> interestADE = makeInterest(fullNameADE);
  shared_ptr<pit::Entry> pitADE = pit.insert(*interestADE).first;

  size_t nNameTreeEntriesBefore = nameTree.size();

  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch(*pitAB).getPrefix(), "/A");
  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch(*pitABC).getPrefix(), fullNameABC);
  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch(*pitADE).getPrefix(), "/A");

  BOOST_CHECK_EQUAL(nameTree.size(), nNameTreeEntriesBefore);
}

BOOST_AUTO_TEST_CASE(LongestPrefixMatchWithMeasurementsEntry)
{
  NameTree nameTree;
  Fib fib(nameTree);

  fib.insert("/A");
  fib.insert("/A/B/C");

  Measurements measurements(nameTree);
  measurements::Entry& mAB = measurements.get("/A/B");
  measurements::Entry& mABCD = measurements.get("/A/B/C/D");

  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch(mAB).getPrefix(), "/A");
  BOOST_CHECK_EQUAL(fib.findLongestPrefixMatch(mABCD).getPrefix(), "/A/B/C");
}

static void
validateFindExactMatch(Fib& fib, const Name& target)
{
  BOOST_TEST_INFO_SCOPE(target);
  const Entry* entry = fib.findExactMatch(target);
  BOOST_REQUIRE(entry != nullptr);
  BOOST_CHECK_EQUAL(entry->getPrefix(), target);
}

static void
validateNoExactMatch(Fib& fib, const Name& target)
{
  BOOST_TEST_INFO_SCOPE(target);
  const Entry* entry = fib.findExactMatch(target);
  BOOST_CHECK(entry == nullptr);
}

BOOST_AUTO_TEST_CASE(ExactMatch)
{
  NameTree nameTree;
  Fib fib(nameTree);
  fib.insert("/A");
  fib.insert("/A/B");
  fib.insert("/A/B/C");

  validateFindExactMatch(fib, "/A");
  validateFindExactMatch(fib, "/A/B");
  validateFindExactMatch(fib, "/A/B/C");

  validateNoExactMatch(fib, "/");
  validateNoExactMatch(fib, "/does/not/exist");
}

BOOST_AUTO_TEST_CASE(ExactMatchGap)
{
  NameTree nameTree;
  Fib fib(nameTree);
  fib.insert("/X");
  fib.insert("/X/Y/Z");

  validateNoExactMatch(fib, "/X/Y");
}

BOOST_AUTO_TEST_CASE(ExactMatchEmpty)
{
  NameTree nameTree;
  Fib fib(nameTree);
  validateNoExactMatch(fib, "/");
  validateNoExactMatch(fib, "/nothing/here");
}

static void
validateErase(Fib& fib, const Name& target)
{
  BOOST_TEST_INFO_SCOPE(target);
  fib.erase(target);
  BOOST_CHECK(fib.findExactMatch(target) == nullptr);
}

BOOST_AUTO_TEST_CASE(Erase)
{
  NameTree nameTree;
  Fib fib(nameTree);
  fib.insert("/");
  fib.insert("/A");
  fib.insert("/A/B");
  fib.insert("/A/B/C");

  // check if we remove the right thing and leave
  // everything else alone

  validateErase(fib, "/A/B");
  validateFindExactMatch(fib, "/A");
  validateFindExactMatch(fib, "/A/B/C");
  validateFindExactMatch(fib, "/");

  validateErase(fib, "/A/B/C");
  validateFindExactMatch(fib, "/A");
  validateFindExactMatch(fib, "/");

  validateErase(fib, "/A");
  validateFindExactMatch(fib, "/");

  validateErase(fib, "/");
  validateNoExactMatch(fib, "/");
}

BOOST_AUTO_TEST_CASE(EraseGap)
{
  NameTree nameTree;
  Fib fib(nameTree);
  fib.insert("/X");
  fib.insert("/X/Y/Z");

  fib.erase("/X/Y"); //should do nothing
  validateFindExactMatch(fib, "/X");
  validateFindExactMatch(fib, "/X/Y/Z");
}

BOOST_AUTO_TEST_CASE(EraseEmpty)
{
  NameTree nameTree;
  Fib fib(nameTree);

  BOOST_CHECK_NO_THROW(fib.erase("/does/not/exist"));
  validateErase(fib, "/");
  BOOST_CHECK_NO_THROW(fib.erase("/still/does/not/exist"));
}

BOOST_AUTO_TEST_CASE(EraseNameTreeEntry)
{
  NameTree nameTree;
  Fib fib(nameTree);
  size_t nNameTreeEntriesBefore = nameTree.size();

  fib.insert("/A/B/C");
  fib.erase("/A/B/C");
  BOOST_CHECK_EQUAL(nameTree.size(), nNameTreeEntriesBefore);
}

BOOST_AUTO_TEST_CASE(Iterator)
{
  NameTree nameTree;
  Fib fib(nameTree);
  Name nameA("/A");
  Name nameAB("/A/B");
  Name nameABC("/A/B/C");
  Name nameRoot("/");

  fib.insert(nameA);
  fib.insert(nameAB);
  fib.insert(nameABC);
  fib.insert(nameRoot);

  std::set<Name> expected{nameA, nameAB, nameABC, nameRoot};

  for (Fib::const_iterator it = fib.begin(); it != fib.end(); it++) {
    bool isInSet = expected.find(it->getPrefix()) != expected.end();
    BOOST_CHECK(isInSet);
    expected.erase(it->getPrefix());
  }

  BOOST_CHECK_EQUAL(expected.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestFib
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace nfd::tests
