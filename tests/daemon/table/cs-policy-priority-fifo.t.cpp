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

#include "table/cs-policy-priority-fifo.hpp"

#include "tests/daemon/table/cs-fixture.hpp"

namespace nfd {
namespace cs {
namespace tests {

BOOST_AUTO_TEST_SUITE(Table)
BOOST_AUTO_TEST_SUITE(TestCsPriorityFifo)

BOOST_AUTO_TEST_CASE(Registration)
{
  std::set<std::string> policyNames = Policy::getPolicyNames();
  BOOST_CHECK_EQUAL(policyNames.count("priority_fifo"), 1);
}

BOOST_FIXTURE_TEST_CASE(EvictOne, CsFixture)
{
  cs.setPolicy(make_unique<PriorityFifoPolicy>());
  cs.setLimit(3);

  insert(1, "/A", [] (Data& data) { data.setFreshnessPeriod(99999_ms); });
  insert(2, "/B", [] (Data& data) { data.setFreshnessPeriod(10_ms); });
  insert(3, "/C", [] (Data& data) { data.setFreshnessPeriod(99999_ms); }, true);
  advanceClocks(11_ms);

  // evict /C (unsolicited)
  insert(4, "/D", [] (Data& data) { data.setFreshnessPeriod(99999_ms); });
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/C");
  CHECK_CS_FIND(0);

  // evict /B (stale)
  insert(5, "/E", [] (Data& data) { data.setFreshnessPeriod(99999_ms); });
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/B");
  CHECK_CS_FIND(0);

  // evict /F (fresh)
  insert(6, "/F", [] (Data& data) { data.setFreshnessPeriod(99999_ms); });
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/A");
  CHECK_CS_FIND(0);
}

BOOST_FIXTURE_TEST_CASE(Refresh, CsFixture)
{
  cs.setPolicy(make_unique<PriorityFifoPolicy>());
  cs.setLimit(3);

  insert(1, "/A", [] (Data& data) { data.setFreshnessPeriod(99999_ms); });
  insert(2, "/B", [] (Data& data) { data.setFreshnessPeriod(10_ms); });
  insert(3, "/C", [] (Data& data) { data.setFreshnessPeriod(10_ms); });
  advanceClocks(11_ms);

  // refresh /B
  insert(12, "/B", [] (Data& data) { data.setFreshnessPeriod(0_ms); });
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/A");
  CHECK_CS_FIND(1);
  startInterest("/B");
  CHECK_CS_FIND(12);
  startInterest("/C");
  CHECK_CS_FIND(3);

  // evict /C from stale queue
  insert(4, "/D", [] (Data& data) { data.setFreshnessPeriod(99999_ms); });
  BOOST_CHECK_EQUAL(cs.size(), 3);
  startInterest("/C");
  CHECK_CS_FIND(0);
}

BOOST_AUTO_TEST_SUITE_END() // TestCsPriorityFifo
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace cs
} // namespace nfd
