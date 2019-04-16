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

#include "table/cs.hpp"

#include "tests/daemon/table/cs-fixture.hpp"

#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace cs {
namespace tests {

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestCs, CsFixture)

BOOST_AUTO_TEST_SUITE(Find)

BOOST_AUTO_TEST_CASE(ExactName)
{
  insert(1, "/");
  insert(2, "/A");
  insert(3, "/A/B");
  insert(4, "/A/C");
  insert(5, "/D");

  startInterest("/A");
  CHECK_CS_FIND(2);
}

BOOST_AUTO_TEST_CASE(ExactName_CanBePrefix)
{
  insert(1, "/");
  insert(2, "/A");
  insert(3, "/A/B");
  insert(4, "/A/C");
  insert(5, "/D");

  startInterest("/A")
    .setCanBePrefix(true);
  CHECK_CS_FIND(2);
}

BOOST_AUTO_TEST_CASE(FullName)
{
  Name n1 = insert(1, "/A");
  Name n2 = insert(2, "/A");

  startInterest(n1);
  CHECK_CS_FIND(1);

  startInterest(n2);
  CHECK_CS_FIND(2);
}

BOOST_AUTO_TEST_CASE(FullName_EmptyDataName)
{
  Name n1 = insert(1, "/");
  Name n2 = insert(2, "/");

  startInterest(n1);
  CHECK_CS_FIND(1);

  startInterest(n2);
  CHECK_CS_FIND(2);
}

BOOST_AUTO_TEST_CASE(PrefixName)
{
  insert(1, "/A");
  insert(2, "/B/p/1");
  insert(3, "/B/p/2");
  insert(4, "/B/q/1");
  insert(5, "/B/q/2");
  insert(6, "/C");

  startInterest("/B")
    .setCanBePrefix(true);
  CHECK_CS_FIND(2);
}

BOOST_AUTO_TEST_CASE(PrefixName_NoCanBePrefix)
{
  insert(1, "/B/p/1");

  startInterest("/B");
  CHECK_CS_FIND(0);
}

BOOST_AUTO_TEST_CASE(MustBeFresh)
{
  insert(1, "/A/1"); // omitted FreshnessPeriod means FreshnessPeriod = 0 ms
  insert(2, "/A/2", [] (Data& data) { data.setFreshnessPeriod(0_s); });
  insert(3, "/A/3", [] (Data& data) { data.setFreshnessPeriod(1_s); });
  insert(4, "/A/4", [] (Data& data) { data.setFreshnessPeriod(1_h); });

  // lookup at exact same moment as insertion is not tested because this won't happen in reality

  advanceClocks(500_ms); // @500ms
  startInterest("/A")
    .setCanBePrefix(true)
    .setMustBeFresh(true);
  CHECK_CS_FIND(3);

  advanceClocks(1500_ms); // @2s
  startInterest("/A")
    .setCanBePrefix(true)
    .setMustBeFresh(true);
  CHECK_CS_FIND(4);

  advanceClocks(3500_s); // @3502s
  startInterest("/A")
    .setCanBePrefix(true)
    .setMustBeFresh(true);
  CHECK_CS_FIND(4);

  advanceClocks(3500_s); // @7002s
  startInterest("/A")
    .setCanBePrefix(true)
    .setMustBeFresh(true);
  CHECK_CS_FIND(0);
}

BOOST_AUTO_TEST_SUITE_END() // Find

BOOST_AUTO_TEST_CASE(Erase)
{
  insert(1, "/A/B/1");
  insert(2, "/A/B/2");
  insert(3, "/A/C/3");
  insert(4, "/A/C/4");
  insert(5, "/D/5");
  insert(6, "/E/6");
  BOOST_CHECK_EQUAL(cs.size(), 6);

  BOOST_CHECK_EQUAL(erase("/A", 3), 3);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  int nDataUnderA = 0;
  startInterest("/A/B/1");
  find([&] (uint32_t found) { nDataUnderA += static_cast<int>(found > 0); });
  startInterest("/A/B/2");
  find([&] (uint32_t found) { nDataUnderA += static_cast<int>(found > 0); });
  startInterest("/A/C/3");
  find([&] (uint32_t found) { nDataUnderA += static_cast<int>(found > 0); });
  startInterest("/A/C/4");
  find([&] (uint32_t found) { nDataUnderA += static_cast<int>(found > 0); });
  BOOST_CHECK_EQUAL(nDataUnderA, 1);

  BOOST_CHECK_EQUAL(erase("/D", 2), 1);
  BOOST_CHECK_EQUAL(cs.size(), 2);
  startInterest("/D/5");
  CHECK_CS_FIND(0);

  BOOST_CHECK_EQUAL(erase("/F", 2), 0);
  BOOST_CHECK_EQUAL(cs.size(), 2);
}

// When the capacity limit is set to zero, Data cannot be inserted;
// this test case covers this situation.
// The behavior of non-zero capacity limit depends on the eviction policy,
// and is tested in policy test suites.
BOOST_AUTO_TEST_CASE(ZeroCapacity)
{
  cs.setLimit(0);

  BOOST_CHECK_EQUAL(cs.getLimit(), 0);
  BOOST_CHECK_EQUAL(cs.size(), 0);
  BOOST_CHECK(cs.begin() == cs.end());

  insert(1, "/A");
  BOOST_CHECK_EQUAL(cs.size(), 0);

  startInterest("/A");
  CHECK_CS_FIND(0);
}

BOOST_AUTO_TEST_CASE(EnablementFlags)
{
  BOOST_CHECK_EQUAL(cs.shouldAdmit(), true);
  BOOST_CHECK_EQUAL(cs.shouldServe(), true);

  insert(1, "/A");
  cs.enableAdmit(false);
  insert(2, "/B");
  cs.enableAdmit(true);
  insert(3, "/C");

  startInterest("/A");
  CHECK_CS_FIND(1);
  startInterest("/B");
  CHECK_CS_FIND(0);
  startInterest("/C");
  CHECK_CS_FIND(3);

  cs.enableServe(false);
  startInterest("/A");
  CHECK_CS_FIND(0);
  startInterest("/C");
  CHECK_CS_FIND(0);

  cs.enableServe(true);
  startInterest("/A");
  CHECK_CS_FIND(1);
  startInterest("/C");
  CHECK_CS_FIND(3);
}

BOOST_AUTO_TEST_CASE(CachePolicyNoCache)
{
  insert(1, "/A", [] (Data& data) {
    data.setTag(make_shared<lp::CachePolicyTag>(
      lp::CachePolicy().setPolicy(lp::CachePolicyType::NO_CACHE)));
  });

  startInterest("/A");
  CHECK_CS_FIND(0);
}

BOOST_AUTO_TEST_CASE(Enumeration)
{
  Name nameA("/A");
  Name nameAB("/A/B");
  Name nameABC("/A/B/C");
  Name nameD("/D");

  BOOST_CHECK_EQUAL(cs.size(), 0);
  BOOST_CHECK(cs.begin() == cs.end());

  insert(123, nameABC);
  BOOST_CHECK_EQUAL(cs.size(), 1);
  BOOST_CHECK(cs.begin() != cs.end());
  BOOST_CHECK(cs.begin()->getName() == nameABC);
  BOOST_CHECK((*cs.begin()).getName() == nameABC);

  auto i = cs.begin();
  auto j = cs.begin();
  BOOST_CHECK(++i == cs.end());
  BOOST_CHECK(j++ == cs.begin());
  BOOST_CHECK(j == cs.end());

  insert(1, nameA);
  insert(12, nameAB);
  insert(4, nameD);

  std::set<Name> expected = {nameA, nameAB, nameABC, nameD};
  std::set<Name> actual;
  for (const auto& csEntry : cs) {
    actual.insert(csEntry.getName());
  }
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END() // TestCs
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace cs
} // namespace nfd
