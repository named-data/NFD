/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "rib/rib-entry.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace rib {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestRibEntry, nfd::tests::BaseFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  rib::RibEntry entry;
  rib::RibEntry::iterator entryIt;
  bool didInsert = false;

  rib::Route route1;
  route1.faceId = 1;
  route1.origin = ndn::nfd::ROUTE_ORIGIN_APP;

  std::tie(entryIt, didInsert) = entry.insertRoute(route1);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 1);
  BOOST_CHECK(entryIt == entry.findRoute(route1));
  BOOST_CHECK(didInsert);

  Route route2;
  route2.faceId = 1;
  route2.origin = ndn::nfd::ROUTE_ORIGIN_NLSR;

  std::tie(entryIt, didInsert) = entry.insertRoute(route2);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 2);
  BOOST_CHECK(entryIt == entry.findRoute(route2));
  BOOST_CHECK(didInsert);

  entry.eraseRoute(route1);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 1);
  BOOST_CHECK(entry.findRoute(route1) == entry.end());

  BOOST_CHECK(entry.findRoute(route1) == entry.getRoutes().end());
  BOOST_CHECK(entry.findRoute(route2) != entry.getRoutes().end());

  std::tie(entryIt, didInsert) = entry.insertRoute(route2);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 1);
  BOOST_CHECK(!didInsert);

  entry.eraseRoute(route1);
  BOOST_CHECK_EQUAL(entry.getRoutes().size(), 1);
  BOOST_CHECK(entry.findRoute(route2) != entry.getRoutes().end());
}

BOOST_AUTO_TEST_SUITE_END() // TestRibEntry

} // namespace tests
} // namespace rib
} // namespace nfd
