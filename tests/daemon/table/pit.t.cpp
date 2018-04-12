/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "table/pit.hpp"

#include "tests/daemon/face/dummy-face.hpp"
#include "tests/test-common.hpp"

#include <ndn-cxx/exclude.hpp>

namespace nfd {
namespace pit {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestPit, BaseFixture)

BOOST_AUTO_TEST_CASE(Find)
{
  shared_ptr<Interest> interest1 = makeInterest("/6hNwxJjw");
  shared_ptr<Interest> interest2 = makeInterest("/v65zqxm4d");

  NameTree nameTree(16);
  Pit pit(nameTree);

  pit.insert(*interest1);
  shared_ptr<pit::Entry> found1a = pit.find(*interest1);
  shared_ptr<pit::Entry> found1b = pit.find(*interest1);
  BOOST_CHECK(found1a != nullptr);
  BOOST_CHECK(found1a == found1b);

  shared_ptr<pit::Entry> found2 = pit.find(*interest2);
  BOOST_CHECK(found2 == nullptr);
  BOOST_CHECK(nameTree.findExactMatch(interest2->getName()) == nullptr);
}

BOOST_AUTO_TEST_CASE(Insert)
{
  Name name1("ndn:/5vzBNnMst");
  Name name2("ndn:/igSGfEIM62");
  ndn::Exclude exclude1;
  exclude1.excludeOne(Name::Component("u26p47oep"));
  ndn::Exclude exclude2;
  exclude2.excludeBefore(Name::Component("u26p47oep"));
  ndn::KeyLocator keyLocator1("ndn:/sGAE3peMHA");
  ndn::KeyLocator keyLocator2("ndn:/nIJH6pr4");

  NameTree nameTree(16);
  Pit pit(nameTree);
  BOOST_CHECK_EQUAL(pit.size(), 0);
  std::pair<shared_ptr<Entry>, bool> insertResult;

  // base
  shared_ptr<Interest> interestA = make_shared<Interest>(name1);
  insertResult = pit.insert(*interestA);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  // same as A
  shared_ptr<Interest> interestA2 = make_shared<Interest>(*interestA);
  insertResult = pit.insert(*interestA2);
  BOOST_CHECK_EQUAL(insertResult.second, false); // sharing the same PIT entry
  BOOST_CHECK_EQUAL(pit.size(), 1);

  // A+MinSuffixComponents
  shared_ptr<Interest> interestB = make_shared<Interest>(*interestA);
  interestB->setMinSuffixComponents(2);
  insertResult = pit.insert(*interestB);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 2);

  // A+MaxSuffixComponents
  shared_ptr<Interest> interestC = make_shared<Interest>(*interestA);
  interestC->setMaxSuffixComponents(4);
  insertResult = pit.insert(*interestC);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 3);

  // A+KeyLocator1
  shared_ptr<Interest> interestD = make_shared<Interest>(*interestA);
  interestD->setPublisherPublicKeyLocator(keyLocator1);
  insertResult = pit.insert(*interestD);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 4);

  // A+KeyLocator2
  shared_ptr<Interest> interestE = make_shared<Interest>(*interestA);
  interestE->setPublisherPublicKeyLocator(keyLocator2);
  insertResult = pit.insert(*interestE);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 5);

  // A+Exclude1
  shared_ptr<Interest> interestF = make_shared<Interest>(*interestA);
  interestF->setExclude(exclude1);
  insertResult = pit.insert(*interestF);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 6);

  // A+Exclude2
  shared_ptr<Interest> interestG = make_shared<Interest>(*interestA);
  interestG->setExclude(exclude2);
  insertResult = pit.insert(*interestG);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 7);

  // A+ChildSelector1
  shared_ptr<Interest> interestI = make_shared<Interest>(*interestA);
  interestI->setChildSelector(1);
  insertResult = pit.insert(*interestI);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 8);

  // A+MustBeFresh
  shared_ptr<Interest> interestJ = make_shared<Interest>(*interestA);
  interestJ->setMustBeFresh(true);
  insertResult = pit.insert(*interestJ);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 9);

  // A+InterestLifetime
  shared_ptr<Interest> interestK = make_shared<Interest>(*interestA);
  interestK->setInterestLifetime(time::milliseconds(1000));
  insertResult = pit.insert(*interestK);
  BOOST_CHECK_EQUAL(insertResult.second, false); // only guiders differ
  BOOST_CHECK_EQUAL(pit.size(), 9);

  // A+Nonce
  shared_ptr<Interest> interestL = make_shared<Interest>(*interestA);
  interestL->setNonce(2192);
  insertResult = pit.insert(*interestL);
  BOOST_CHECK_EQUAL(insertResult.second, false); // only guiders differ
  BOOST_CHECK_EQUAL(pit.size(), 9);

  // different Name+Exclude1
  shared_ptr<Interest> interestM = make_shared<Interest>(name2);
  interestM->setExclude(exclude1);
  insertResult = pit.insert(*interestM);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 10);
}

BOOST_AUTO_TEST_CASE(Erase)
{
  shared_ptr<Interest> interest = makeInterest("/z88Admz6A2");

  NameTree nameTree(16);
  Pit pit(nameTree);

  std::pair<shared_ptr<Entry>, bool> insertResult;

  BOOST_CHECK_EQUAL(pit.size(), 0);

  insertResult = pit.insert(*interest);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.find(*interest) != nullptr);

  insertResult = pit.insert(*interest);
  BOOST_CHECK_EQUAL(insertResult.second, false);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.find(*interest) != nullptr);

  pit.erase(insertResult.first.get());
  BOOST_CHECK_EQUAL(pit.size(), 0);
  BOOST_CHECK(pit.find(*interest) == nullptr);

  insertResult = pit.insert(*interest);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.find(*interest) != nullptr);
}

BOOST_AUTO_TEST_CASE(EraseNameTreeEntry)
{
  NameTree nameTree;
  Pit pit(nameTree);
  size_t nNameTreeEntriesBefore = nameTree.size();

  shared_ptr<Interest> interest = makeInterest("/37xWVvQ2K");
  shared_ptr<Entry> entry = pit.insert(*interest).first;
  pit.erase(entry.get());
  BOOST_CHECK_EQUAL(nameTree.size(), nNameTreeEntriesBefore);
}

BOOST_AUTO_TEST_CASE(EraseWithFullName)
{
  shared_ptr<Data> data = makeData("/test");
  shared_ptr<Interest> interest = makeInterest(data->getFullName());

  NameTree nameTree(16);
  Pit pit(nameTree);

  BOOST_CHECK_EQUAL(pit.size(), 0);

  BOOST_CHECK_EQUAL(pit.insert(*interest).second, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.find(*interest) != nullptr);

  BOOST_CHECK_EQUAL(pit.insert(*interest).second, false);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  shared_ptr<pit::Entry> pitEntry = pit.find(*interest);
  BOOST_REQUIRE(pitEntry != nullptr);

  pit.erase(pitEntry.get());
  BOOST_CHECK_EQUAL(pit.size(), 0);
  BOOST_CHECK(pit.find(*interest) == nullptr);

  BOOST_CHECK_EQUAL(pit.insert(*interest).second, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.find(*interest) != nullptr);
}

BOOST_AUTO_TEST_CASE(FindAllDataMatches)
{
  Name nameA   ("ndn:/A");
  Name nameAB  ("ndn:/A/B");
  Name nameABC ("ndn:/A/B/C");
  Name nameABCD("ndn:/A/B/C/D");
  Name nameD   ("ndn:/D");

  shared_ptr<Interest> interestA   = makeInterest(nameA  );
  shared_ptr<Interest> interestABC = makeInterest(nameABC);
  shared_ptr<Interest> interestD   = makeInterest(nameD  );

  NameTree nameTree(16);
  Pit pit(nameTree);
  int count = 0;

  BOOST_CHECK_EQUAL(pit.size(), 0);

  pit.insert(*interestA  );
  pit.insert(*interestABC);
  pit.insert(*interestD  );

  nameTree.lookup(nameABCD); // make sure /A/B/C/D is in nameTree

  BOOST_CHECK_EQUAL(pit.size(), 3);

  shared_ptr<Data> data = makeData(nameABCD);

  DataMatchResult matches = pit.findAllDataMatches(*data);

  bool hasA   = false;
  bool hasAB  = false;
  bool hasABC = false;
  bool hasD   = false;

  for (const shared_ptr<Entry>& entry : matches) {
    ++count;

    if (entry->getName().equals(nameA ))
      hasA   = true;

    if (entry->getName().equals(nameAB))
      hasAB  = true;

    if (entry->getName().equals(nameABC))
      hasABC = true;

    if (entry->getName().equals(nameD))
      hasD   = true;
  }
  BOOST_CHECK_EQUAL(hasA  , true);
  BOOST_CHECK_EQUAL(hasAB , false);
  BOOST_CHECK_EQUAL(hasABC, true);
  BOOST_CHECK_EQUAL(hasD  , false);

  BOOST_CHECK_EQUAL(count, 2);
}

BOOST_AUTO_TEST_CASE(MatchFullName) // Bug 3363
{
  NameTree nameTree(16);
  Pit pit(nameTree);

  shared_ptr<Data> data = makeData("/A");
  Name fullName = data->getFullName();
  shared_ptr<Interest> interest = makeInterest(fullName);

  pit.insert(*interest);
  DataMatchResult matches = pit.findAllDataMatches(*data);

  BOOST_REQUIRE_EQUAL(std::distance(matches.begin(), matches.end()), 1);
  shared_ptr<Entry> found = *matches.begin();
  BOOST_CHECK_EQUAL(found->getName(), fullName);
}

BOOST_AUTO_TEST_CASE(InsertMatchLongName)
{
  NameTree nameTree(16);
  Pit pit(nameTree);

  Name n1;
  while (n1.size() < NameTree::getMaxDepth()) {
    n1.append("A");
  }
  Name n2 = n1;
  while (n2.size() < NameTree::getMaxDepth() * 2) {
    n2.append("B");
  }
  Name n3 = n1;
  while (n3.size() < NameTree::getMaxDepth() * 2) {
    n3.append("C");
  }
  auto d2 = makeData(n2);
  auto i2 = makeInterest(n2);
  auto d3 = makeData(n3);
  auto i3 = makeInterest(d3->getFullName());

  shared_ptr<Entry> entry2 = pit.insert(*i2).first;
  shared_ptr<Entry> entry3 = pit.insert(*i3).first;

  BOOST_CHECK_EQUAL(pit.size(), 2);
  BOOST_CHECK_EQUAL(nameTree.size(), 1 + NameTree::getMaxDepth()); // root node + max depth
  BOOST_CHECK(entry2->getInterest().matchesInterest(*i2));
  BOOST_CHECK(entry3->getInterest().matchesInterest(*i3));

  DataMatchResult matches2 = pit.findAllDataMatches(*d2);
  BOOST_REQUIRE_EQUAL(std::distance(matches2.begin(), matches2.end()), 1);
  BOOST_CHECK(*matches2.begin() == entry2);
  DataMatchResult matches3 = pit.findAllDataMatches(*d3);
  BOOST_REQUIRE_EQUAL(std::distance(matches3.begin(), matches3.end()), 1);
  BOOST_CHECK(*matches3.begin() == entry3);
}

BOOST_AUTO_TEST_CASE(Iterator)
{
  NameTree nameTree(16);
  Pit pit(nameTree);

  shared_ptr<Interest> interestA    = makeInterest("/A");
  shared_ptr<Interest> interestABC1 = makeInterest("/A/B/C");
  shared_ptr<Interest> interestABC2 = makeInterest("/A/B/C");
  interestABC2->setSelectors(ndn::Selectors().setMinSuffixComponents(10));
  shared_ptr<Interest> interestD    = makeInterest("/D");

  BOOST_CHECK_EQUAL(pit.size(), 0);
  BOOST_CHECK(pit.begin() == pit.end());

  pit.insert(*interestABC1);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.begin() != pit.end());
  BOOST_CHECK(pit.begin()->getInterest() == *interestABC1);
  BOOST_CHECK((*pit.begin()).getInterest() == *interestABC1);

  auto i = pit.begin();
  auto j = pit.begin();
  BOOST_CHECK(++i == pit.end());
  BOOST_CHECK(j++ == pit.begin());
  BOOST_CHECK(j == pit.end());

  pit.insert(*interestA);
  pit.insert(*interestABC2);
  pit.insert(*interestD);

  std::set<const Interest*> expected = {&*interestA, &*interestABC1, &*interestABC2, &*interestD};
  std::set<const Interest*> actual;
  for (const auto& pitEntry : pit) {
    actual.insert(&pitEntry.getInterest());
  }
  BOOST_CHECK(actual == expected);
  for (auto actualIt = actual.begin(), expectedIt = expected.begin();
       actualIt != actual.end() && expectedIt != expected.end(); ++actualIt, ++expectedIt) {
    BOOST_CHECK_EQUAL(**actualIt, **expectedIt);
  }
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace pit
} // namespace nfd
