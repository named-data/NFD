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

#include "rib/readvertise/client-to-nlsr-readvertise-policy.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Readvertise)
BOOST_FIXTURE_TEST_SUITE(TestClientToNlsrReadvertisePolicy, BaseFixture)

BOOST_AUTO_TEST_CASE(ReadvertiseClientRoute)
{
  auto entry = make_shared<RibEntry>();
  entry->setName("/test/A");
  Route route;
  route.origin = ndn::nfd::ROUTE_ORIGIN_CLIENT;
  auto routeIt = entry->insertRoute(route).first;
  RibRouteRef rrr{entry, routeIt};

  ClientToNlsrReadvertisePolicy policy;
  optional<ReadvertiseAction> action = policy.handleNewRoute(rrr);

  BOOST_REQUIRE(action);
  BOOST_CHECK_EQUAL(action->prefix, "/test/A");
  BOOST_REQUIRE_EQUAL(action->signer, ndn::security::SigningInfo());
}

BOOST_AUTO_TEST_CASE(DontReadvertiseRoute)
{
  auto entry = make_shared<RibEntry>();
  entry->setName("/test/A");
  Route route;
  route.origin = ndn::nfd::ROUTE_ORIGIN_NLSR;
  auto routeIt = entry->insertRoute(route).first;
  RibRouteRef rrr{entry, routeIt};

  ClientToNlsrReadvertisePolicy policy;
  optional<ReadvertiseAction> action = policy.handleNewRoute(rrr);

  BOOST_CHECK(!action);
}

BOOST_AUTO_TEST_SUITE_END() // TestClientToNlsrReadvertisePolicy
BOOST_AUTO_TEST_SUITE_END() // Readvertise

} // namespace tests
} // namespace rib
} // namespace nfd
