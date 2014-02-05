/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/fib.hpp"
#include "../face/dummy-face.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(TableFib)

BOOST_AUTO_TEST_CASE(Entry)
{
  Name prefix("ndn:/pxWhfFza");
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  
  fib::Entry entry(prefix);
  BOOST_CHECK(entry.getPrefix().equals(prefix));
  
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

  Fib fib;
  // ['/']
  
  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_CHECK_NE(entry.get(), static_cast<fib::Entry*>(0));
  BOOST_CHECK(entry->getPrefix().equals(nameEmpty));
  
  insertRes = fib.insert(nameA);
  BOOST_CHECK_EQUAL(insertRes.second, true);
  BOOST_CHECK(insertRes.first->getPrefix().equals(nameA));
  // ['/', '/A']
  
  insertRes = fib.insert(nameA);
  BOOST_CHECK_EQUAL(insertRes.second, false);
  BOOST_CHECK(insertRes.first->getPrefix().equals(nameA));
  // ['/', '/A']

  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_CHECK_NE(entry.get(), static_cast<fib::Entry*>(0));
  BOOST_CHECK(entry->getPrefix().equals(nameA));
  
  entry = fib.findLongestPrefixMatch(nameABCD);
  BOOST_CHECK_NE(entry.get(), static_cast<fib::Entry*>(0));
  BOOST_CHECK(entry->getPrefix().equals(nameA));
  
  insertRes = fib.insert(nameABC);
  BOOST_CHECK_EQUAL(insertRes.second, true);
  BOOST_CHECK(insertRes.first->getPrefix().equals(nameABC));
  // ['/', '/A', '/A/B/C']

  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_CHECK_NE(entry.get(), static_cast<fib::Entry*>(0));
  BOOST_CHECK(entry->getPrefix().equals(nameA));
  
  entry = fib.findLongestPrefixMatch(nameAB);
  BOOST_CHECK_NE(entry.get(), static_cast<fib::Entry*>(0));
  BOOST_CHECK(entry->getPrefix().equals(nameA));
  
  entry = fib.findLongestPrefixMatch(nameABCD);
  BOOST_CHECK_NE(entry.get(), static_cast<fib::Entry*>(0));
  BOOST_CHECK(entry->getPrefix().equals(nameABC));
  
  entry = fib.findLongestPrefixMatch(nameE);
  BOOST_CHECK_NE(entry.get(), static_cast<fib::Entry*>(0));
  BOOST_CHECK(entry->getPrefix().equals(nameEmpty));
}

BOOST_AUTO_TEST_CASE(RemoveNextHopFromAllEntries)
{
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  Name nameA("ndn:/A");
  Name nameB("ndn:/B");
  
  std::pair<shared_ptr<fib::Entry>, bool> insertRes;
  shared_ptr<fib::Entry> entry;
  
  Fib fib;
  // {'/':[]}
  
  insertRes = fib.insert(nameA);
  insertRes.first->addNextHop(face1, 0);
  insertRes.first->addNextHop(face2, 0);
  // {'/':[], '/A':[1,2]}

  insertRes = fib.insert(nameB);
  insertRes.first->addNextHop(face1, 0);
  // {'/':[], '/A':[1,2], '/B':[1]}
  
  fib.removeNextHopFromAllEntries(face1);
  // {'/':[], '/A':[2], '/B':[]}
  
  entry = fib.findLongestPrefixMatch(nameA);
  BOOST_CHECK(entry->getPrefix().equals(nameA));
  const fib::NextHopList& nexthopsA = entry->getNextHops();
  BOOST_CHECK_EQUAL(nexthopsA.size(), 1);
  BOOST_CHECK_EQUAL(nexthopsA.begin()->getFace(), face2);
  
  entry = fib.findLongestPrefixMatch(nameB);
  BOOST_CHECK(entry->getPrefix().equals(nameB));
  const fib::NextHopList& nexthopsB = entry->getNextHops();
  BOOST_CHECK_EQUAL(nexthopsB.size(), 0);
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
  Fib fib;
  fib.insert("/A");
  fib.insert("/A/B");
  fib.insert("/A/B/C");

  validateFindExactMatch(fib, "/A");
  validateFindExactMatch(fib, "/A/B");
  validateFindExactMatch(fib, "/A/B/C");
  validateFindExactMatch(fib, "/");

  validateNoExactMatch(fib, "/does/not/exist");

  Fib gapFib;
  fib.insert("/X");
  fib.insert("/X/Y/Z");

  validateNoExactMatch(gapFib, "/X/Y");

  Fib emptyFib;
  validateNoExactMatch(emptyFib, "/nothing/here");
}

void
validateRemove(Fib& fib, const Name& target)
{
  fib.remove(target);

  shared_ptr<fib::Entry> entry = fib.findExactMatch(target);
  if (static_cast<bool>(entry))
    {
      BOOST_FAIL("Found \"removed\" entry for " << target);
    }
}

BOOST_AUTO_TEST_CASE(Remove)
{
  Fib emptyFib;

  emptyFib.remove("/does/not/exist"); // crash test

  validateRemove(emptyFib, "/");

  emptyFib.remove("/still/does/not/exist"); // crash test

  Fib fib;
  fib.insert("/A");
  fib.insert("/A/B");
  fib.insert("/A/B/C");

  // check if we remove the right thing and leave
  // everything else alone

  validateRemove(fib, "/A/B");
  validateFindExactMatch(fib, "/A");
  validateFindExactMatch(fib, "/A/B/C");
  validateFindExactMatch(fib, "/");

  validateRemove(fib, "/A/B/C");
  validateFindExactMatch(fib, "/A");
  validateFindExactMatch(fib, "/");

  validateRemove(fib, "/A");
  validateFindExactMatch(fib, "/");

  Fib gapFib;
  gapFib.insert("/X");
  gapFib.insert("/X/Y/Z");

  gapFib.remove("/X/Y"); //should do nothing
  validateFindExactMatch(gapFib, "/X");
  validateFindExactMatch(gapFib, "/X/Y/Z");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
