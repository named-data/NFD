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

#include "rib/rib-entry.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

BOOST_FIXTURE_TEST_SUITE(TestRibEntry, GlobalIoFixture)

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

BOOST_FIXTURE_TEST_SUITE(GetAnnouncement, GlobalIoTimeFixture)

static Route
makeSimpleRoute(uint64_t faceId)
{
  Route route;
  route.faceId = faceId;
  return route;
}

static Route
makeSimpleRoute(uint64_t faceId, time::steady_clock::Duration expiration)
{
  Route route = makeSimpleRoute(faceId);
  route.expires = time::steady_clock::now() + expiration;
  return route;
}

BOOST_AUTO_TEST_CASE(Empty)
{
  RibEntry entry;
  // entry has no Route

  advanceClocks(1_s);
  auto pa = entry.getPrefixAnnouncement(15_s, 30_s);
  BOOST_CHECK_GE(pa.getExpiration(), 15_s);
  BOOST_CHECK_LE(pa.getExpiration(), 30_s);
  // A RibEntry without Route does not have a defined expiration time,
  // so this test only checks that there's no exception and the expiration period is clamped.
}

BOOST_AUTO_TEST_CASE(ReuseAnnouncement1)
{
  RibEntry entry;
  entry.insertRoute(Route(makePrefixAnn("/A", 212_s), 5858));
  entry.insertRoute(Route(makePrefixAnn("/A", 555_s), 2454));
  entry.insertRoute(makeSimpleRoute(8282, 799_s));
  entry.insertRoute(makeSimpleRoute(1141));

  advanceClocks(1_s);
  auto pa = entry.getPrefixAnnouncement(15_s, 30_s);
  BOOST_CHECK_EQUAL(pa.getExpiration(), 555_s); // not modified and not clamped
}

BOOST_AUTO_TEST_CASE(ReuseAnnouncement2)
{
  RibEntry entry;
  entry.insertRoute(Route(makePrefixAnn("/A", 212_s), 7557)); // expires at +212s

  advanceClocks(100_s);
  entry.insertRoute(Route(makePrefixAnn("/A", 136_s), 1010)); // expires at +236s

  advanceClocks(1_s);
  auto pa = entry.getPrefixAnnouncement(15_s, 30_s);
  BOOST_CHECK_EQUAL(pa.getExpiration(), 136_s);
  // Although face 7557's announcement has a longer expiration period,
  // the route for face 1010 expires last and thus its announcement is chosen.
}

BOOST_AUTO_TEST_CASE(MakeAnnouncement)
{
  RibEntry entry;
  entry.insertRoute(makeSimpleRoute(6398, 765_s));
  entry.insertRoute(makeSimpleRoute(2954, 411_s));

  advanceClocks(1_s);
  auto pa = entry.getPrefixAnnouncement(15_s, 999_s);
  BOOST_CHECK_EQUAL(pa.getExpiration(), 764_s);
}

BOOST_AUTO_TEST_CASE(MakeAnnouncementInfinite)
{
  RibEntry entry;
  entry.insertRoute(makeSimpleRoute(4877, 240_s));
  entry.insertRoute(makeSimpleRoute(5053));

  advanceClocks(1_s);
  auto pa = entry.getPrefixAnnouncement(15_s, 877_s);
  BOOST_CHECK_EQUAL(pa.getExpiration(), 877_s); // clamped at maxExpiration
}

BOOST_AUTO_TEST_CASE(MakeAnnouncementShortExpiration)
{
  RibEntry entry;
  entry.insertRoute(makeSimpleRoute(4877, 8_s));
  entry.insertRoute(makeSimpleRoute(5053, 9_s));

  advanceClocks(1_s);
  auto pa = entry.getPrefixAnnouncement(15_s, 666_s);
  BOOST_CHECK_EQUAL(pa.getExpiration(), 15_s); // clamped at minExpiration
}

BOOST_AUTO_TEST_SUITE_END() // GetAnnouncement

BOOST_AUTO_TEST_SUITE_END() // TestRibEntry

} // namespace tests
} // namespace rib
} // namespace nfd
