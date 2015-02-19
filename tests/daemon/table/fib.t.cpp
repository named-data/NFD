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

#include "table/fib.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TableFib, BaseFixture)

BOOST_AUTO_TEST_CASE(Entry)
{
  Name prefix("ndn:/pxWhfFza");
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();

  fib::Entry entry(prefix);
  BOOST_CHECK_EQUAL(entry.getPrefix(), prefix);

  const fib::NextHopList& nexthops1 = entry.getNextHops();
  // []
  BOOST_CHECK_EQUAL(nexthops1.size(), 0);

  entry.addNextHop(face1, 20);
  const fib::NextHopList& nexthops2 = entry.getNextHops();
  // [(face1,20)]
  BOOST_CHECK_EQUAL(nexthops2.size(), 1);
  BOOST_CHECK_EQUAL(nexthops2.begin()->getFace(), face1);
  BOOST_CHECK_EQUAL(nexthops2.begin()->getCost(), 20);

  entry.addNextHop(face1, 30);
  const fib::NextHopList& nexthops3 = entry.getNextHops();
  // [(face1,30)]
  BOOST_CHECK_EQUAL(nexthops3.size(), 1);
  BOOST_CHECK_EQUAL(nexthops3.begin()->getFace(), face1);
  BOOST_CHECK_EQUAL(nexthops3.begin()->getCost(), 30);

  entry.addNextHop(face2, 40);
  const fib::NextHopList& nexthops4 = entry.getNextHops();
  // [(face1,30), (face2,40)]
  BOOST_CHECK_EQUAL(nexthops4.size(), 2);
  int i = -1;
  for (fib::NextHopList::const_iterator it = nexthops4.begin();
    it != nexthops4.end(); ++it) {
    ++i;
    switch (i) {
      case 0 :
        BOOST_CHECK_EQUAL(it->getFace(), face1);
        BOOST_CHECK_EQUAL(it->getCost(), 30);
        break;
      case 1 :
        BOOST_CHECK_EQUAL(it->getFace(), face2);
        BOOST_CHECK_EQUAL(it->getCost(), 40);
        break;
    }
  }

  entry.addNextHop(face2, 10);
  const fib::NextHopList& nexthops5 = entry.getNextHops();
  // [(face2,10), (face1,30)]
  BOOST_CHECK_EQUAL(nexthops5.size(), 2);
  i = -1;
  for (fib::NextHopList::const_iterator it = nexthops5.begin();
    it != nexthops5.end(); ++it) {
    ++i;
    switch (i) {
      case 0 :
        BOOST_CHECK_EQUAL(it->getFace(), face2);
        BOOST_CHECK_EQUAL(it->getCost(), 10);
        break;
      case 1 :
        BOOST_CHECK_EQUAL(it->getFace(), face1);
        BOOST_CHECK_EQUAL(it->getCost(), 30);
        break;
    }
  }

  entry.removeNextHop(face1);
  const fib::NextHopList& nexthops6 = entry.getNextHops();
  // [(face2,10)]
  BOOST_CHECK_EQUAL(nexthops6.size(), 1);
  BOOST_CHECK_EQUAL(nexthops6.begin()->getFace(), face2);
  BOOST_CHECK_EQUAL(nexthops6.begin()->getCost(), 10);

  entry.removeNextHop(face1);
  const fib::NextHopList& nexthops7 = entry.getNextHops();
  // [(face2,10)]
  BOOST_CHECK_EQUAL(nexthops7.size(), 1);
  BOOST_CHECK_EQUAL(nexthops7.begin()->getFace(), face2);
  BOOST_CHECK_EQUAL(nexthops7.begin()->getCost(), 10);

  entry.removeNextHop(face2);
  const fib::NextHopList& nexthops8 = entry.getNextHops();
  // []
  BOOST_CHECK_EQUAL(nexthops8.size(), 0);

  entry.removeNextHop(face2);
  const fib::NextHopList& nexthops9 = entry.getNextHops();
  // []
  BOOST_CHECK_EQUAL(nexthops9.size(), 0);
}

BOOST_AUTO_TEST_CASE(Insert_LongestPrefixMatch)
{
  Name nameEmpty;
  Name nameA   ("ndn:/A");
  Name nameAB  ("ndn:/A/B");
  Name nameABC ("ndn:/A/B/C");
  Name nameABCD("ndn:/A/B/C/D");
  Name nameE   ("ndn:/E");

  std::pair<shared_ptr<fib::Entry>, bool> insertRes;
  shared_ptr<fib::Entry> entry;

  NameTree nameTree;
  Fib fib(nameTree);
  // []
  BOOST_CHECK_EQUAL(fib.size(), 0);

  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_REQUIRE(static_cast<bool>(entry)); // the empty entry

  insertRes = fib.insert(nameEmpty);
  BOOST_CHECK_EQUAL(insertRes.second, true);
  BOOST_CHECK_EQUAL(insertRes.first->getPrefix(), nameEmpty);
  // ['/']
  BOOST_CHECK_EQUAL(fib.size(), 1);

  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_REQUIRE(static_cast<bool>(entry));
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameEmpty);

  insertRes = fib.insert(nameA);
  BOOST_CHECK_EQUAL(insertRes.second, true);
  BOOST_CHECK_EQUAL(insertRes.first->getPrefix(), nameA);
  // ['/', '/A']
  BOOST_CHECK_EQUAL(fib.size(), 2);

  insertRes = fib.insert(nameA);
  BOOST_CHECK_EQUAL(insertRes.second, false);
  BOOST_CHECK_EQUAL(insertRes.first->getPrefix(), nameA);
  // ['/', '/A']
  BOOST_CHECK_EQUAL(fib.size(), 2);

  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_REQUIRE(static_cast<bool>(entry));
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameA);

  entry = fib.findLongestPrefixMatch(nameABCD);
  BOOST_REQUIRE(static_cast<bool>(entry));
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameA);

  insertRes = fib.insert(nameABC);
  BOOST_CHECK_EQUAL(insertRes.second, true);
  BOOST_CHECK_EQUAL(insertRes.first->getPrefix(), nameABC);
  // ['/', '/A', '/A/B/C']
  BOOST_CHECK_EQUAL(fib.size(), 3);

  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_REQUIRE(static_cast<bool>(entry));
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameA);

  entry = fib.findLongestPrefixMatch(nameAB);
  BOOST_REQUIRE(static_cast<bool>(entry));
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameA);

  entry = fib.findLongestPrefixMatch(nameABCD);
  BOOST_REQUIRE(static_cast<bool>(entry));
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameABC);

  entry = fib.findLongestPrefixMatch(nameE);
  BOOST_REQUIRE(static_cast<bool>(entry));
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameEmpty);
}

BOOST_AUTO_TEST_CASE(RemoveNextHopFromAllEntries)
{
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  Name nameEmpty("ndn:/");
  Name nameA("ndn:/A");
  Name nameB("ndn:/B");

  std::pair<shared_ptr<fib::Entry>, bool> insertRes;
  shared_ptr<fib::Entry> entry;

  NameTree nameTree;
  Fib fib(nameTree);
  // {}

  insertRes = fib.insert(nameA);
  insertRes.first->addNextHop(face1, 0);
  insertRes.first->addNextHop(face2, 0);
  // {'/A':[1,2]}

  insertRes = fib.insert(nameB);
  insertRes.first->addNextHop(face1, 0);
  // {'/A':[1,2], '/B':[1]}
  BOOST_CHECK_EQUAL(fib.size(), 2);

  insertRes = fib.insert("/C");
  insertRes.first->addNextHop(face2, 1);
  // {'/A':[1,2], '/B':[1], '/C':[2]}
  BOOST_CHECK_EQUAL(fib.size(), 3);

  insertRes = fib.insert("/B/1");
  insertRes.first->addNextHop(face1, 0);
  // {'/A':[1,2], '/B':[1], '/B/1':[1], '/C':[2]}
  BOOST_CHECK_EQUAL(fib.size(), 4);

  insertRes = fib.insert("/B/1/2");
  insertRes.first->addNextHop(face1, 0);
  // {'/A':[1,2], '/B':[1], '/B/1':[1], '/B/1/2':[1], '/C':[2]}
  BOOST_CHECK_EQUAL(fib.size(), 5);

  insertRes = fib.insert("/B/1/2/3");
  insertRes.first->addNextHop(face1, 0);
  // {'/A':[1,2], '/B':[1], '/B/1':[1], '/B/1/2':[1], '/B/1/3':[1], '/C':[2]}
  BOOST_CHECK_EQUAL(fib.size(), 6);

  insertRes = fib.insert("/B/1/2/3/4");
  insertRes.first->addNextHop(face1, 0);
  // {'/A':[1,2], '/B':[1], '/B/1':[1], '/B/1/2':[1], '/B/1/2/3':[1], '/B/1/2/3/4':[1], '/C':[2]}
  BOOST_CHECK_EQUAL(fib.size(), 7);

  /////////////

  fib.removeNextHopFromAllEntries(face1);
  // {'/A':[2], '/C':[2]}
  BOOST_CHECK_EQUAL(fib.size(), 2);

  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameA);
  const fib::NextHopList& nexthopsA = entry->getNextHops();
  BOOST_CHECK_EQUAL(nexthopsA.size(), 1);
  BOOST_CHECK_EQUAL(nexthopsA.begin()->getFace(), face2);

  entry = fib.findLongestPrefixMatch(nameB);
  BOOST_CHECK_EQUAL(entry->getPrefix(), nameEmpty);
}

BOOST_AUTO_TEST_CASE(RemoveNextHopFromManyEntries)
{
  NameTree nameTree(16);
  Fib fib(nameTree);
  shared_ptr<Face> face1 = make_shared<DummyFace>();

  for (uint64_t i = 0; i < 300; ++i) {
    shared_ptr<fib::Entry> entry = fib.insert(Name("/P").appendVersion(i)).first;
    entry->addNextHop(face1, 0);
  }
  BOOST_CHECK_EQUAL(fib.size(), 300);

  fib.removeNextHopFromAllEntries(face1);
  BOOST_CHECK_EQUAL(fib.size(), 0);
}

void
validateFindExactMatch(const Fib& fib, const Name& target)
{
  shared_ptr<fib::Entry> entry = fib.findExactMatch(target);
  if (static_cast<bool>(entry))
    {
      BOOST_CHECK_EQUAL(entry->getPrefix(), target);
    }
  else
    {
      BOOST_FAIL("No entry found for " << target);
    }
}

void
validateNoExactMatch(const Fib& fib, const Name& target)
{
  shared_ptr<fib::Entry> entry = fib.findExactMatch(target);
  if (static_cast<bool>(entry))
    {
      BOOST_FAIL("Found unexpected entry for " << target);
    }
}

BOOST_AUTO_TEST_CASE(FindExactMatch)
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

  NameTree gapNameTree;
  Fib gapFib(nameTree);
  fib.insert("/X");
  fib.insert("/X/Y/Z");

  validateNoExactMatch(gapFib, "/X/Y");

  NameTree emptyNameTree;
  Fib emptyFib(emptyNameTree);
  validateNoExactMatch(emptyFib, "/nothing/here");
}

void
validateErase(Fib& fib, const Name& target)
{
  fib.erase(target);

  shared_ptr<fib::Entry> entry = fib.findExactMatch(target);
  if (static_cast<bool>(entry))
    {
      BOOST_FAIL("Found \"removed\" entry for " << target);
    }
}

BOOST_AUTO_TEST_CASE(Erase)
{
  NameTree emptyNameTree;
  Fib emptyFib(emptyNameTree);

  emptyFib.erase("/does/not/exist"); // crash test

  validateErase(emptyFib, "/");

  emptyFib.erase("/still/does/not/exist"); // crash test

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

  NameTree gapNameTree;
  Fib gapFib(gapNameTree);
  gapFib.insert("/X");
  gapFib.insert("/X/Y/Z");

  gapFib.erase("/X/Y"); //should do nothing
  validateFindExactMatch(gapFib, "/X");
  validateFindExactMatch(gapFib, "/X/Y/Z");
}

BOOST_AUTO_TEST_CASE(EraseNameTreeEntry)
{
  NameTree nameTree;
  Fib fib(nameTree);
  size_t nNameTreeEntriesBefore = nameTree.size();

  fib.insert("ndn:/A/B/C");
  fib.erase("ndn:/A/B/C");
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

  std::set<Name> expected;
  expected.insert(nameA);
  expected.insert(nameAB);
  expected.insert(nameABC);
  expected.insert(nameRoot);

  for (Fib::const_iterator it = fib.begin(); it != fib.end(); it++)
  {
    bool isInSet = expected.find(it->getPrefix()) != expected.end();
    BOOST_CHECK(isInSet);
    expected.erase(it->getPrefix());
  }

  BOOST_CHECK_EQUAL(expected.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
