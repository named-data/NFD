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

#include "table/measurements.hpp"
#include "table/fib.hpp"
#include "table/pit.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace measurements {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Table)

class MeasurementsFixture : public UnitTestTimeFixture
{
public:
  MeasurementsFixture()
    : measurements(nameTree)
  {
  }

public:
  NameTree nameTree;
  Measurements measurements;
};

BOOST_FIXTURE_TEST_SUITE(TestMeasurements, MeasurementsFixture)

BOOST_AUTO_TEST_CASE(Get_Parent)
{
  Entry& entryAB = measurements.get("/A/B");
  BOOST_CHECK_EQUAL(entryAB.getName(), "/A/B");

  Entry& entry0 = measurements.get("/");
  BOOST_CHECK_EQUAL(entry0.getName(), "/");

  Entry* entryA = measurements.getParent(entryAB);
  BOOST_REQUIRE(entryA != nullptr);
  BOOST_CHECK_EQUAL(entryA->getName(), "/A");

  Entry* entry0c = measurements.getParent(*entryA);
  BOOST_REQUIRE(entry0c != nullptr);
  BOOST_CHECK_EQUAL(&entry0, entry0c);
}

BOOST_AUTO_TEST_CASE(GetLongName)
{
  Name n;
  while (n.size() < NameTree::getMaxDepth() - 1) {
    n.append("A");
  }
  Entry& entry1 = measurements.get(n);
  BOOST_CHECK_EQUAL(entry1.getName().size(), NameTree::getMaxDepth() - 1);

  n.append("B");
  Entry& entry2 = measurements.get(n);
  BOOST_CHECK_EQUAL(entry2.getName().size(), NameTree::getMaxDepth());

  n.append("C");
  Entry& entry3 = measurements.get(n);
  BOOST_CHECK_EQUAL(entry3.getName().size(), NameTree::getMaxDepth());
}

BOOST_AUTO_TEST_CASE(GetWithFibEntry)
{
  Fib fib(nameTree);

  const fib::Entry* fibA = fib.insert("/A").first;
  const fib::Entry* fibAB = fib.insert("/A/B").first;

  Entry& entryA = measurements.get(*fibA);
  BOOST_CHECK_EQUAL(entryA.getName(), "/A");

  Entry& entryAB = measurements.get(*fibAB);
  BOOST_CHECK_EQUAL(entryAB.getName(), "/A/B");
}

BOOST_AUTO_TEST_CASE(GetWithEmptyFibEntry) // Bug 3275
{
  Fib fib(nameTree);

  const fib::Entry& fib0 = fib.findLongestPrefixMatch("/");

  Entry& entry0 = measurements.get(fib0);
  BOOST_CHECK_EQUAL(entry0.getName(), "/");
}

BOOST_AUTO_TEST_CASE(GetWithPitEntry)
{
  Pit pit(nameTree);

  shared_ptr<Interest> interestA = makeInterest("/A");
  shared_ptr<pit::Entry> pitA = pit.insert(*interestA).first;
  shared_ptr<Data> dataABC = makeData("/A/B/C");
  Name fullName = dataABC->getFullName();
  shared_ptr<Interest> interestFull = makeInterest(fullName);
  shared_ptr<pit::Entry> pitFull = pit.insert(*interestFull).first;

  Entry& entryA = measurements.get(*pitA);
  BOOST_CHECK_EQUAL(entryA.getName(), "/A");

  Entry& entryFull = measurements.get(*pitFull);
  BOOST_CHECK_EQUAL(entryFull.getName(), fullName);
}

class DummyStrategyInfo1 : public fw::StrategyInfo
{
public:
  static constexpr int
  getTypeId()
  {
    return 21;
  }
};

class DummyStrategyInfo2 : public fw::StrategyInfo
{
public:
  static constexpr int
  getTypeId()
  {
    return 22;
  }
};

BOOST_AUTO_TEST_CASE(FindLongestPrefixMatch)
{
  measurements.get("/A");
  measurements.get("/A/B/C").insertStrategyInfo<DummyStrategyInfo1>();
  measurements.get("/A/B/C/D");

  Entry* found1 = measurements.findLongestPrefixMatch("/A/B/C/D/E");
  BOOST_REQUIRE(found1 != nullptr);
  BOOST_CHECK_EQUAL(found1->getName(), "/A/B/C/D");

  Entry* found2 = measurements.findLongestPrefixMatch("/A/B/C/D/E",
      EntryWithStrategyInfo<DummyStrategyInfo1>());
  BOOST_REQUIRE(found2 != nullptr);
  BOOST_CHECK_EQUAL(found2->getName(), "/A/B/C");

  Entry* found3 = measurements.findLongestPrefixMatch("/A/B/C/D/E",
      EntryWithStrategyInfo<DummyStrategyInfo2>());
  BOOST_CHECK(found3 == nullptr);
}

BOOST_AUTO_TEST_CASE(FindLongestPrefixMatchWithPitEntry)
{
  Pit pit(nameTree);

  measurements.get("/A");
  measurements.get("/A/B/C").insertStrategyInfo<DummyStrategyInfo1>();
  measurements.get("/A/B/C/D");

  shared_ptr<Interest> interest = makeInterest("/A/B/C/D/E");
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;

  Entry* found1 = measurements.findLongestPrefixMatch(*pitEntry);
  BOOST_REQUIRE(found1 != nullptr);
  BOOST_CHECK_EQUAL(found1->getName(), "/A/B/C/D");

  Entry* found2 = measurements.findLongestPrefixMatch(*pitEntry,
      EntryWithStrategyInfo<DummyStrategyInfo1>());
  BOOST_REQUIRE(found2 != nullptr);
  BOOST_CHECK_EQUAL(found2->getName(), "/A/B/C");

  Entry* found3 = measurements.findLongestPrefixMatch(*pitEntry,
      EntryWithStrategyInfo<DummyStrategyInfo2>());
  BOOST_CHECK(found3 == nullptr);
}

BOOST_AUTO_TEST_CASE(Lifetime)
{
  Name nameA("ndn:/A");
  Name nameB("ndn:/B");
  Name nameC("ndn:/C");

  BOOST_CHECK_EQUAL(measurements.size(), 0);

  Entry& entryA = measurements.get(nameA);
  measurements.get(nameB);
  Entry& entryC = measurements.get(nameC);
  BOOST_CHECK_EQUAL(measurements.size(), 3);

  const time::nanoseconds EXTEND_A = time::seconds(2);
  const time::nanoseconds CHECK1 = time::seconds(3);
  const time::nanoseconds CHECK2 = time::seconds(5);
  const time::nanoseconds EXTEND_C = time::seconds(6);
  const time::nanoseconds CHECK3 = time::seconds(7);
  BOOST_ASSERT(EXTEND_A < CHECK1);
  BOOST_ASSERT(CHECK1 < Measurements::getInitialLifetime());
  BOOST_ASSERT(Measurements::getInitialLifetime() < CHECK2);
  BOOST_ASSERT(CHECK2 < EXTEND_C);
  BOOST_ASSERT(EXTEND_C < CHECK3);

  measurements.extendLifetime(entryA, EXTEND_A);
  measurements.extendLifetime(entryC, EXTEND_C);
  // remaining lifetime:
  //   A = initial lifetime, because it's extended by less duration
  //   B = initial lifetime
  //   C = EXTEND_C

  this->advanceClocks(time::milliseconds(100), CHECK1);
  BOOST_CHECK(measurements.findExactMatch(nameA) != nullptr);
  BOOST_CHECK(measurements.findExactMatch(nameB) != nullptr);
  BOOST_CHECK(measurements.findExactMatch(nameC) != nullptr);
  BOOST_CHECK_EQUAL(measurements.size(), 3);

  this->advanceClocks(time::milliseconds(100), CHECK2 - CHECK1);
  BOOST_CHECK(measurements.findExactMatch(nameA) == nullptr);
  BOOST_CHECK(measurements.findExactMatch(nameB) == nullptr);
  BOOST_CHECK(measurements.findExactMatch(nameC) != nullptr);
  BOOST_CHECK_EQUAL(measurements.size(), 1);

  this->advanceClocks(time::milliseconds(100), CHECK3 - CHECK2);
  BOOST_CHECK(measurements.findExactMatch(nameA) == nullptr);
  BOOST_CHECK(measurements.findExactMatch(nameB) == nullptr);
  BOOST_CHECK(measurements.findExactMatch(nameC) == nullptr);
  BOOST_CHECK_EQUAL(measurements.size(), 0);
}

BOOST_AUTO_TEST_CASE(EraseNameTreeEntry)
{
  size_t nNameTreeEntriesBefore = nameTree.size();

  measurements.get("/A");
  BOOST_CHECK_EQUAL(measurements.size(), 1);

  this->advanceClocks(Measurements::getInitialLifetime() + time::milliseconds(10));
  BOOST_CHECK_EQUAL(measurements.size(), 0);
  BOOST_CHECK_EQUAL(nameTree.size(), nNameTreeEntriesBefore);
}

BOOST_AUTO_TEST_SUITE_END() // TestMeasurements
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace measurements
} // namespace nfd
