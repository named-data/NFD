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

#include "table/cs-policy-lru.hpp"

#include "tests/daemon/table/cs-fixture.hpp"

namespace nfd {
namespace cs {
namespace tests {

BOOST_AUTO_TEST_SUITE(Table)
BOOST_AUTO_TEST_SUITE(TestCsLru)

BOOST_AUTO_TEST_CASE(Registration)
{
  std::set<std::string> policyNames = Policy::getPolicyNames();
  BOOST_CHECK_EQUAL(policyNames.count("lru"), 1);
}

BOOST_FIXTURE_TEST_CASE(EvictOne, CsFixture)
{
  cs.setPolicy(make_unique<LruPolicy>());
  cs.setLimit(3);

  insert(1, "/A");
  insert(2, "/B");
  insert(3, "/C");
  BOOST_CHECK_EQUAL(cs.size(), 3);

  // evict A
  insert(4, "/D");
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/A");
  CHECK_CS_FIND(0);

  // use C then B
  startInterest("/C");
  CHECK_CS_FIND(3);
  startInterest("/B");
  CHECK_CS_FIND(2);

  // evict D then C
  insert(5, "/E");
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/D");
  CHECK_CS_FIND(0);
  insert(6, "/F");
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/C");
  CHECK_CS_FIND(0);

  // refresh B
  insert(12, "/B");
  // evict E
  insert(7, "/G");
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/E");
  CHECK_CS_FIND(0);
}

BOOST_AUTO_TEST_SUITE_END() // TestCsLru
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace cs
} // namespace nfd
