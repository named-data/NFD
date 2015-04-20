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

#define CHECK_CS_FIND(expected) find([&] (uint32_t found) { BOOST_CHECK_EQUAL(expected, found); });

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

  void
  find(const std::function<void(uint32_t)>& check)
  {
    m_cs.find(*m_interest,
              [&] (const Interest& interest, const Data& data) {
                  const Block& content = data.getContent();
                  uint32_t found = *reinterpret_cast<const uint32_t*>(content.value());
                  check(found); },
              bind([&] { check(0); }));
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
  CHECK_CS_FIND(1);
}

BOOST_AUTO_TEST_CASE(EmptyInterestName)
{
  insert(1, "ndn:/A");

  startInterest("ndn:/");
  CHECK_CS_FIND(1);
}

BOOST_AUTO_TEST_CASE(ExactName)
{
  insert(1, "ndn:/");
  insert(2, "ndn:/A");
  insert(3, "ndn:/A/B");
  insert(4, "ndn:/A/C");
  insert(5, "ndn:/D");

  startInterest("ndn:/A");
  CHECK_CS_FIND(2);
}

BOOST_AUTO_TEST_CASE(FullName)
{
  Name n1 = insert(1, "ndn:/A");
  Name n2 = insert(2, "ndn:/A");

  startInterest(n1);
  CHECK_CS_FIND(1);

  startInterest(n2);
  CHECK_CS_FIND(2);
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
  CHECK_CS_FIND(2);
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
  CHECK_CS_FIND(4);
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
  CHECK_CS_FIND(1);

  startInterest("ndn:/")
    .setMinSuffixComponents(1);
  CHECK_CS_FIND(1);

  startInterest("ndn:/")
    .setMinSuffixComponents(2);
  CHECK_CS_FIND(2);

  startInterest("ndn:/")
    .setMinSuffixComponents(3);
  CHECK_CS_FIND(3);

  startInterest("ndn:/")
    .setMinSuffixComponents(4);
  CHECK_CS_FIND(4);

  startInterest("ndn:/")
    .setMinSuffixComponents(5);
  CHECK_CS_FIND(5);

  startInterest("ndn:/")
    .setMinSuffixComponents(6);
  CHECK_CS_FIND(6);

  startInterest("ndn:/")
    .setMinSuffixComponents(7);
  CHECK_CS_FIND(0);
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
  CHECK_CS_FIND(0);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(1);
  CHECK_CS_FIND(1);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(2);
  CHECK_CS_FIND(2);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(3);
  CHECK_CS_FIND(3);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(4);
  CHECK_CS_FIND(4);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(5);
  CHECK_CS_FIND(5);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(6);
  CHECK_CS_FIND(6);

  startInterest("ndn:/")
    .setChildSelector(1)
    .setMaxSuffixComponents(7);
  CHECK_CS_FIND(6);
}

BOOST_AUTO_TEST_CASE(DigestOrder)
{
  insert(1, "ndn:/A");
  insert(2, "ndn:/A");
  // We don't know which comes first, but there must be some order

  int leftmost = 0, rightmost = 0;
  startInterest("ndn:/A")
    .setChildSelector(0);
  m_cs.find(*m_interest,
            [&leftmost] (const Interest& interest, const Data& data) {
              leftmost = *reinterpret_cast<const uint32_t*>(data.getContent().value());},
            bind([] { BOOST_CHECK(false); }));
  startInterest("ndn:/A")
    .setChildSelector(1);
  m_cs.find(*m_interest,
            [&rightmost] (const Interest, const Data& data) {
              rightmost = *reinterpret_cast<const uint32_t*>(data.getContent().value()); },
            bind([] { BOOST_CHECK(false); }));
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
  CHECK_CS_FIND(3);

  startInterest("ndn:/A")
    .setChildSelector(1)
    .setExclude(excludeDigest);
  CHECK_CS_FIND(3);

  Exclude excludeGeneric;
  excludeGeneric.excludeAfter(name::Component(static_cast<uint8_t*>(nullptr), 0));

  startInterest("ndn:/A")
    .setChildSelector(0)
    .setExclude(excludeGeneric);
  find([] (uint32_t found) { BOOST_CHECK(found == 1 || found == 2); });

  startInterest("ndn:/A")
    .setChildSelector(1)
    .setExclude(excludeGeneric);
  find([] (uint32_t found) { BOOST_CHECK(found == 1 || found == 2); });

  Exclude exclude2 = excludeGeneric;
  exclude2.excludeOne(n2.get(-1));

  startInterest("ndn:/A")
    .setChildSelector(0)
    .setExclude(exclude2);
  CHECK_CS_FIND(1);

  startInterest("ndn:/A")
    .setChildSelector(1)
    .setExclude(exclude2);
  CHECK_CS_FIND(1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(CachingPolicyNoCache)
{
  Cs cs(3);

  shared_ptr<Data> dataA = makeData("ndn:/A");
  dataA->getLocalControlHeader().setCachingPolicy(LocalControlHeader::CachingPolicy::NO_CACHE);
  dataA->wireEncode();
  BOOST_CHECK_EQUAL(cs.insert(*dataA), false);

  cs.find(Interest("ndn:/A"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));
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
