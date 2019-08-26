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

#include "table/pit.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace pit {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestPit, GlobalIoFixture)

BOOST_AUTO_TEST_CASE(Find)
{
  auto interest1 = makeInterest("/6hNwxJjw");
  auto interest2 = makeInterest("/v65zqxm4d");

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
  Name name1("/5vzBNnMst");
  Name name2("/igSGfEIM62");

  NameTree nameTree(16);
  Pit pit(nameTree);
  BOOST_CHECK_EQUAL(pit.size(), 0);

  shared_ptr<Entry> entry;
  bool isNew = false;

  // base
  auto interestA = makeInterest(name1, false, nullopt, 2148);
  std::tie(entry, isNew) = pit.insert(*interestA);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  // same Interest, same PIT entry
  auto interestA2 = make_shared<Interest>(*interestA);
  std::tie(entry, isNew) = pit.insert(*interestA2);
  BOOST_CHECK_EQUAL(isNew, false);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  // different CanBePrefix, different PIT entry
  auto interestB = make_shared<Interest>(*interestA);
  interestB->setCanBePrefix(true);
  std::tie(entry, isNew) = pit.insert(*interestB);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK_EQUAL(pit.size(), 2);

  // different MustBeFresh, different PIT entry
  auto interestC = make_shared<Interest>(*interestA);
  interestC->setMustBeFresh(true);
  std::tie(entry, isNew) = pit.insert(*interestC);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK_EQUAL(pit.size(), 3);

  // different InterestLifetime, same PIT entry
  auto interestD = make_shared<Interest>(*interestA);
  interestD->setInterestLifetime(1_s);
  std::tie(entry, isNew) = pit.insert(*interestD);
  BOOST_CHECK_EQUAL(isNew, false);
  BOOST_CHECK_EQUAL(pit.size(), 3);

  // different Nonce, same PIT entry
  auto interestE = make_shared<Interest>(*interestA);
  interestE->setNonce(2192);
  std::tie(entry, isNew) = pit.insert(*interestE);
  BOOST_CHECK_EQUAL(isNew, false);
  BOOST_CHECK_EQUAL(pit.size(), 3);

  // different name, different PIT entry
  auto interestF = make_shared<Interest>(*interestA);
  interestF->setName(name2);
  std::tie(entry, isNew) = pit.insert(*interestF);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK_EQUAL(pit.size(), 4);
}

BOOST_AUTO_TEST_CASE(Erase)
{
  auto interest = makeInterest("/z88Admz6A2");

  NameTree nameTree(16);
  Pit pit(nameTree);

  shared_ptr<Entry> entry;
  bool isNew = false;

  BOOST_CHECK_EQUAL(pit.size(), 0);

  std::tie(entry, isNew) = pit.insert(*interest);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.find(*interest) != nullptr);

  std::tie(entry, isNew) = pit.insert(*interest);
  BOOST_CHECK_EQUAL(isNew, false);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.find(*interest) != nullptr);

  pit.erase(entry.get());
  BOOST_CHECK_EQUAL(pit.size(), 0);
  BOOST_CHECK(pit.find(*interest) == nullptr);

  std::tie(entry, isNew) = pit.insert(*interest);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.find(*interest) != nullptr);
}

BOOST_AUTO_TEST_CASE(EraseNameTreeEntry)
{
  NameTree nameTree;
  Pit pit(nameTree);
  size_t nNameTreeEntriesBefore = nameTree.size();

  auto interest = makeInterest("/37xWVvQ2K");
  auto entry = pit.insert(*interest).first;
  pit.erase(entry.get());
  BOOST_CHECK_EQUAL(nameTree.size(), nNameTreeEntriesBefore);
}

BOOST_AUTO_TEST_CASE(EraseWithFullName)
{
  auto data = makeData("/test");
  auto interest = makeInterest(data->getFullName());

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
  Name nameA   ("/A");
  Name nameAB  ("/A/B");
  Name nameABC ("/A/B/C");
  Name nameABCD("/A/B/C/D");
  Name nameD   ("/D");

  auto interestA   = makeInterest(nameA,   true);
  auto interestABC = makeInterest(nameABC, true);
  auto interestD   = makeInterest(nameD,   true);

  NameTree nameTree(16);
  Pit pit(nameTree);
  int count = 0;

  BOOST_CHECK_EQUAL(pit.size(), 0);

  pit.insert(*interestA  );
  pit.insert(*interestABC);
  pit.insert(*interestD  );

  nameTree.lookup(nameABCD); // make sure /A/B/C/D is in nameTree

  BOOST_CHECK_EQUAL(pit.size(), 3);

  auto data = makeData(nameABCD);

  DataMatchResult matches = pit.findAllDataMatches(*data);

  bool hasA   = false;
  bool hasAB  = false;
  bool hasABC = false;
  bool hasD   = false;

  for (const auto& entry : matches) {
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

  auto data = makeData("/A");
  Name fullName = data->getFullName();
  auto interest = makeInterest(fullName);

  pit.insert(*interest);
  DataMatchResult matches = pit.findAllDataMatches(*data);

  BOOST_REQUIRE_EQUAL(std::distance(matches.begin(), matches.end()), 1);
  auto found = *matches.begin();
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

  auto entry2 = pit.insert(*i2).first;
  auto entry3 = pit.insert(*i3).first;

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

  auto interestA = makeInterest("/A");
  auto interestABC1 = makeInterest("/A/B/C");
  auto interestABC2 = makeInterest("/A/B/C");
  interestABC2->setMustBeFresh(true);
  auto interestD = makeInterest("/D");

  BOOST_CHECK_EQUAL(pit.size(), 0);
  BOOST_CHECK(pit.begin() == pit.end());

  pit.insert(*interestABC1);
  BOOST_CHECK_EQUAL(pit.size(), 1);
  BOOST_CHECK(pit.begin() != pit.end());
  BOOST_CHECK(&pit.begin()->getInterest() == interestABC1.get());
  BOOST_CHECK(&(*pit.begin()).getInterest() == interestABC1.get());

  auto i = pit.begin();
  auto j = pit.begin();
  BOOST_CHECK(++i == pit.end());
  BOOST_CHECK(j++ == pit.begin());
  BOOST_CHECK(j == pit.end());

  pit.insert(*interestA);
  pit.insert(*interestABC2);
  pit.insert(*interestD);

  std::set<const Interest*> actual;
  for (const auto& pitEntry : pit) {
    actual.insert(&pitEntry.getInterest());
  }
  std::set<const Interest*> expected = {interestA.get(), interestABC1.get(),
                                        interestABC2.get(), interestD.get()};
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END() // TestPit
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace pit
} // namespace nfd
