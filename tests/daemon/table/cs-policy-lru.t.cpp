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
#include "table/cs-policy-lru.hpp"
#include <ndn-cxx/util/crypto.hpp>

#include "tests/test-common.hpp"

namespace nfd {
namespace cs {
namespace tests {

using namespace nfd::tests;
using ndn::nfd::LocalControlHeader;

BOOST_AUTO_TEST_SUITE(CsLru)

BOOST_FIXTURE_TEST_CASE(EvictOneLRU, UnitTestTimeFixture)
{
  Cs cs(3);
  cs.setPolicy(unique_ptr<Policy>(new LruPolicy()));

  cs.insert(*makeData("ndn:/A"));
  cs.insert(*makeData("ndn:/B"));
  cs.insert(*makeData("ndn:/C"));
  BOOST_CHECK_EQUAL(cs.size(), 3);

  // evict A
  cs.insert(*makeData("ndn:/D"));
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/A"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));

  // use C then B
  cs.find(Interest("ndn:/C"),
          bind([] { BOOST_CHECK(true); }),
          bind([] { BOOST_CHECK(false); }));
  cs.find(Interest("ndn:/B"),
          bind([] { BOOST_CHECK(true); }),
          bind([] { BOOST_CHECK(false); }));

  // evict D then C
  cs.insert(*makeData("ndn:/E"));
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/D"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));
  cs.insert(*makeData("ndn:/F"));
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/C"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));

  // refresh B
  cs.insert(*makeData("ndn:/B"));
  // evict E
  cs.insert(*makeData("ndn:/G"));
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/E"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace cs
} // namespace nfd
