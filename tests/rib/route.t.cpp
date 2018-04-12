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

#include "rib/route.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

BOOST_FIXTURE_TEST_SUITE(TestRoute, BaseFixture)

BOOST_AUTO_TEST_CASE(Equality)
{
  Route a;
  Route b;
  BOOST_CHECK_EQUAL(a, b);

  a.faceId = b.faceId = 15404;
  a.origin = b.origin = ndn::nfd::ROUTE_ORIGIN_NLSR;
  a.flags = b.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;
  a.cost = b.cost = 28826;
  a.expires = b.expires = time::steady_clock::now() + time::milliseconds(26782232);
  BOOST_CHECK_EQUAL(a, b);

  b.faceId = 18559;
  BOOST_CHECK_NE(a, b);
  a.faceId = 18559;

  b.origin = ndn::nfd::ROUTE_ORIGIN_CLIENT;
  BOOST_CHECK_NE(a, b);
  a.origin = ndn::nfd::ROUTE_ORIGIN_CLIENT;

  b.flags = ndn::nfd::ROUTE_FLAG_CAPTURE;
  BOOST_CHECK_NE(a, b);
  a.flags = ndn::nfd::ROUTE_FLAG_CAPTURE;

  b.cost = 103;
  BOOST_CHECK_NE(a, b);
  a.cost = 103;

  b.expires = nullopt;
  BOOST_CHECK_NE(a, b);
  a.expires = nullopt;

  BOOST_CHECK_EQUAL(a, b);
}

BOOST_FIXTURE_TEST_CASE(Output, UnitTestTimeFixture)
{
  Route r;
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),
                    "Route(faceid: 0, origin: app, cost: 0, flags: 0x0, never expires)");

  r.faceId = 4980;
  r.origin = ndn::nfd::ROUTE_ORIGIN_STATIC;
  r.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;
  r.cost = 2312;
  r.expires = time::steady_clock::now() + time::milliseconds(791214234);
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),
                    "Route(faceid: 4980, origin: static, cost: 2312, flags: 0x1, expires in: 791214234 milliseconds)");

  r.expires = nullopt;
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),
                    "Route(faceid: 4980, origin: static, cost: 2312, flags: 0x1, never expires)");
}

BOOST_AUTO_TEST_SUITE_END() // TestRoute

} // namespace tests
} // namespace rib
} // namespace nfd
