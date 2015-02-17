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

#include "table/cs.hpp"
#include <ndn-cxx/util/crypto.hpp>

#include "tests/test-common.hpp"

namespace nfd {
namespace cs {
namespace tests {

using namespace nfd::tests;
using ndn::nfd::LocalControlHeader;

BOOST_FIXTURE_TEST_SUITE(TableCs, BaseFixture)

class FindFixture : protected BaseFixture
{
protected:
  Name
  insert(uint32_t id, const Name& name)
  {
    shared_ptr<Data> data = makeData(name);
    data->setFreshnessPeriod(time::milliseconds(99999));
    data->setContent(reinterpret_cast<const uint8_t*>(&id), sizeof(id));
    data->wireEncode();

    m_cs.insert(*data);

    return data->getFullName();
  }

  Interest&
  startInterest(const Name& name)
  {
    m_interest = make_shared<Interest>(name);
    return *m_interest;
  }

  uint32_t
  find()
  {
    // m_cs.dump();

    const Data* found = m_cs.find(*m_interest);
    if (found == nullptr) {
      return 0;
    }
    const Block& content = found->getContent();
    if (content.value_size() != sizeof(uint32_t)) {
      return 0;
    }
    return *reinterpret_cast<const uint32_t*>(content.value());
  }

protected:
  Cs m_cs;
  shared_ptr<Interest> m_interest;
};

BOOST_FIXTURE_TEST_SUITE(Find, FindFixture)

BOOST_AUTO_TEST_CASE(EmptyDataName)
{
  insert(1, "ndn:/");

  startInterest("ndn:/");
  BOOST_CHECK_EQUAL(find(), 1);
}

BOOST_AUTO_TEST_CASE(EmptyInterestName)
{
  insert(1, "ndn:/A");

  startInterest("ndn:/");
  BOOST_CHECK_EQUAL(find(), 1);
}

BOOST_AUTO_TEST_CASE(ExactName)
{
  insert(1, "ndn:/");
  insert(2, "ndn:/A");
  insert(3, "ndn:/A/B");
  insert(4, "ndn:/A/C");
  insert(5, "ndn:/D");

  startInterest("ndn:/A");
  BOOST_CHECK_EQUAL(find(), 2);
}

BOOST_AUTO_TEST_CASE(FullName)
{
  Name n1 = insert(1, "ndn:/A");
  Name n2 = insert(2, "ndn:/A");

  startInterest(n1);
  BOOST_CHECK_EQUAL(find(), 1);

  startInterest(n2);
  BOOST_CHECK_EQUAL(find(), 2);
}

BOOST_AUTO_TEST_CASE(Leftmost)
{
  insert(1, "ndn:/A");
  insert(2, "ndn:/B/p/1");
  insert(3, "ndn:/B/p/2");
  insert(4, "ndn:/B/q/1");
  insert(5, "ndn:/B/q/2");
  insert(6, "ndn:/C");

  startInterest("ndn:/B");
  BOOST_CHECK_EQUAL(find(), 2);
}

BOOST_AUTO_TEST_CASE(Rightmost)
{
  insert(1, "ndn:/A");
  insert(2, "ndn:/B/p/1");
  insert(3, "ndn:/B/p/2");
  insert(4, "ndn:/B/q/1");
  insert(5, "ndn:/B/q/2");
  insert(6, "ndn:/C");

  startInterest("ndn:/B")
    .setChildSelector(1);
  BOOST_CHECK_EQUAL(find(), 4);
}

BOOST_AUTO_TEST_CASE(MinSuffixComponents)
{
  insert(1, "ndn:/");
  insert(2, "ndn:/A");
  insert(3, "ndn:/B/1");
  insert(4, "ndn:/C/1/2");
  insert(5, "ndn:/D/1/2/3");
  insert(6, "ndn:/E/1/2/3/4");

  startInterest("ndn:/")
    .setMinSuffixComponents(0);
  BOOST_CHECK_EQUAL(find(), 1);

  startInterest("ndn:/")
    .setMinSuffixComponents(1);
  BOOST_CHECK_EQUAL(find(), 1);

  startInterest("ndn:/")
    .setMinSuffixComponents(2);
  BOOST_CHECK_EQUAL(find(), 2);

  startInterest("ndn:/")
    .setMinSuffixComponents(3);
  BOOST_CHECK_EQUAL(find(), 3);

  startInterest("ndn:/")
    .setMinSuffixComponents(4);
  BOOST_CHECK_EQUAL(find(), 4);

  startInterest("ndn:/")
    .setMinSuffixComponents(5);
  BOOST_CHECK_EQUAL(find(), 5);

  startInterest("ndn:/")
    .setMinSuffixComponents(6);
  BOOST_CHECK_EQUAL(find(), 6);

  startInterest("ndn:/")
    .setMinSuffixComponents(7);
  BOOST_CHECK_EQUAL(find(), 0);
}

BOOST_AUTO_TEST_CASE(MaxSuffixComponents)
{
  insert(1, "ndn:/");
  insert(2, "ndn:/A");
  insert(3, "ndn:/B/2");
  insert(4, "ndn:/C/2/3");
  insert(5, "ndn:/D/2/3/4");
  insert(6, "ndn:/E/2/3/4/5");

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(0);
  BOOST_CHECK_EQUAL(find(), 0);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(1);
  BOOST_CHECK_EQUAL(find(), 1);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(2);
  BOOST_CHECK_EQUAL(find(), 2);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(3);
  BOOST_CHECK_EQUAL(find(), 3);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(4);
  BOOST_CHECK_EQUAL(find(), 4);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(5);
  BOOST_CHECK_EQUAL(find(), 5);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(6);
  BOOST_CHECK_EQUAL(find(), 6);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(7);
  BOOST_CHECK_EQUAL(find(), 6);
}

BOOST_AUTO_TEST_CASE(DigestOrder)
{
  insert(1, "ndn:/A");
  insert(2, "ndn:/A");
  // We don't know which comes first, but there must be some order

  startInterest("ndn:/A")
    .setChildSelector(0);
  uint32_t leftmost = find();

  startInterest("ndn:/A")
    .setChildSelector(1);
  uint32_t rightmost = find();

  BOOST_CHECK_NE(leftmost, rightmost);
}

BOOST_AUTO_TEST_CASE(DigestExclude)
{
  insert(1, "ndn:/A");
  Name n2 = insert(2, "ndn:/A");
  insert(3, "ndn:/A/B");

  uint8_t digest00[ndn::crypto::SHA256_DIGEST_SIZE];
  std::fill_n(digest00, sizeof(digest00), 0x00);
  uint8_t digestFF[ndn::crypto::SHA256_DIGEST_SIZE];
  std::fill_n(digestFF, sizeof(digestFF), 0xFF);

  Exclude excludeDigest;
  excludeDigest.excludeRange(
    name::Component::fromImplicitSha256Digest(digest00, sizeof(digest00)),
    name::Component::fromImplicitSha256Digest(digestFF, sizeof(digestFF)));

  startInterest("ndn:/A")
    .setChildSelector(0)
    .setExclude(excludeDigest);
  BOOST_CHECK_EQUAL(find(), 3);

  startInterest("ndn:/A")
    .setChildSelector(1)
    .setExclude(excludeDigest);
  BOOST_CHECK_EQUAL(find(), 3);

  Exclude excludeGeneric;
  excludeGeneric.excludeAfter(name::Component(static_cast<uint8_t*>(nullptr), 0));

  startInterest("ndn:/A")
    .setChildSelector(0)
    .setExclude(excludeGeneric);
  int found1 = find();
  BOOST_CHECK(found1 == 1 || found1 == 2);

  startInterest("ndn:/A")
    .setChildSelector(1)
    .setExclude(excludeGeneric);
  int found2 = find();
  BOOST_CHECK(found2 == 1 || found2 == 2);

  Exclude exclude2 = excludeGeneric;
  exclude2.excludeOne(n2.get(-1));

  startInterest("ndn:/A")
    .setChildSelector(0)
    .setExclude(exclude2);
  BOOST_CHECK_EQUAL(find(), 1);

  startInterest("ndn:/A")
    .setChildSelector(1)
    .setExclude(exclude2);
  BOOST_CHECK_EQUAL(find(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(CachingPolicyNoCache)
{
  Cs cs(3);

  shared_ptr<Data> dataA = makeData("ndn:/A");
  dataA->getLocalControlHeader().setCachingPolicy(LocalControlHeader::CachingPolicy::NO_CACHE);
  dataA->wireEncode();
  BOOST_CHECK_EQUAL(cs.insert(*dataA), false);

  BOOST_CHECK(cs.find(Interest("ndn:/A")) == nullptr);
}

BOOST_FIXTURE_TEST_CASE(Evict, UnitTestTimeFixture)
{
  Cs cs(3);

  shared_ptr<Data> dataA = makeData("ndn:/A");
  dataA->setFreshnessPeriod(time::milliseconds(99999));
  dataA->wireEncode();
  cs.insert(*dataA);

  shared_ptr<Data> dataB = makeData("ndn:/B");
  dataB->setFreshnessPeriod(time::milliseconds(10));
  dataB->wireEncode();
  cs.insert(*dataB);

  shared_ptr<Data> dataC = makeData("ndn:/C");
  dataC->setFreshnessPeriod(time::milliseconds(99999));
  dataC->wireEncode();
  cs.insert(*dataC, true);

  this->advanceClocks(time::milliseconds(11));

  // evict unsolicited
  shared_ptr<Data> dataD = makeData("ndn:/D");
  dataD->setFreshnessPeriod(time::milliseconds(99999));
  dataD->wireEncode();
  cs.insert(*dataD);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  BOOST_CHECK(cs.find(Interest("ndn:/C")) == nullptr);

  // evict stale
  shared_ptr<Data> dataE = makeData("ndn:/E");
  dataE->setFreshnessPeriod(time::milliseconds(99999));
  dataE->wireEncode();
  cs.insert(*dataE);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  BOOST_CHECK(cs.find(Interest("ndn:/B")) == nullptr);

  // evict fifo
  shared_ptr<Data> dataF = makeData("ndn:/F");
  dataF->setFreshnessPeriod(time::milliseconds(99999));
  dataF->wireEncode();
  cs.insert(*dataF);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  BOOST_CHECK(cs.find(Interest("ndn:/A")) == nullptr);
}

BOOST_FIXTURE_TEST_CASE(Refresh, UnitTestTimeFixture)
{
  Cs cs(3);

  shared_ptr<Data> dataA = makeData("ndn:/A");
  dataA->setFreshnessPeriod(time::milliseconds(99999));
  dataA->wireEncode();
  cs.insert(*dataA);

  shared_ptr<Data> dataB = makeData("ndn:/B");
  dataB->setFreshnessPeriod(time::milliseconds(10));
  dataB->wireEncode();
  cs.insert(*dataB);

  shared_ptr<Data> dataC = makeData("ndn:/C");
  dataC->setFreshnessPeriod(time::milliseconds(10));
  dataC->wireEncode();
  cs.insert(*dataC);

  this->advanceClocks(time::milliseconds(11));

  // refresh dataB
  shared_ptr<Data> dataB2 = make_shared<Data>(*dataB);
  dataB2->wireEncode();
  cs.insert(*dataB2);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  BOOST_CHECK(cs.find(Interest("ndn:/A")) != nullptr);
  BOOST_CHECK(cs.find(Interest("ndn:/B")) != nullptr);
  BOOST_CHECK(cs.find(Interest("ndn:/C")) != nullptr);

  // evict dataC stale
  shared_ptr<Data> dataD = makeData("ndn:/D");
  dataD->setFreshnessPeriod(time::milliseconds(99999));
  dataD->wireEncode();
  cs.insert(*dataD);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  BOOST_CHECK(cs.find(Interest("ndn:/C")) == nullptr);
}

BOOST_AUTO_TEST_CASE(Enumeration)
{
  Cs cs;

  Name nameA("/A");
  Name nameAB("/A/B");
  Name nameABC("/A/B/C");
  Name nameD("/D");

  BOOST_CHECK_EQUAL(cs.size(), 0);
  BOOST_CHECK(cs.begin() == cs.end());

  cs.insert(*makeData(nameABC));
  BOOST_CHECK_EQUAL(cs.size(), 1);
  BOOST_CHECK(cs.begin() != cs.end());
  BOOST_CHECK(cs.begin()->getName() == nameABC);
  BOOST_CHECK((*cs.begin()).getName() == nameABC);

  auto i = cs.begin();
  auto j = cs.begin();
  BOOST_CHECK(++i == cs.end());
  BOOST_CHECK(j++ == cs.begin());
  BOOST_CHECK(j == cs.end());

  cs.insert(*makeData(nameA));
  cs.insert(*makeData(nameAB));
  cs.insert(*makeData(nameD));

  std::set<Name> expected = {nameA, nameAB, nameABC, nameD};
  std::set<Name> actual;
  for (const auto& csEntry : cs) {
    actual.insert(csEntry.getName());
  }
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace cs
} // namespace nfd
