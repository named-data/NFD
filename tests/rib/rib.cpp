/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology,
 *                     The University of Memphis
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
 **/

#include "rib/rib.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace rib {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(Rib, nfd::tests::BaseFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  rib::Rib rib;

  RibEntry entry1;
  entry1.name = "/hello/world";
  entry1.faceId = 1;
  entry1.origin = 20;
  entry1.cost = 10;
  entry1.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT | ndn::nfd::ROUTE_FLAG_CAPTURE;
  entry1.expires = time::steady_clock::now() + time::milliseconds(1500);

  rib.insert(entry1);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  rib.insert(entry1);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  RibEntry entry2;
  entry2.name = "/hello/world";
  entry2.faceId = 1;
  entry2.origin = 20;
  entry2.cost = 100;
  entry2.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;
  entry2.expires = time::steady_clock::now() + time::seconds(0);

  rib.insert(entry2);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  entry2.faceId = 2;
  rib.insert(entry2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  entry2.name = "/foo/bar";
  rib.insert(entry2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  entry2.origin = 1;
  rib.insert(entry2);
  BOOST_CHECK_EQUAL(rib.size(), 4);

  rib.erase(entry2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  entry2.name = "/hello/world";
  rib.erase(entry2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  entry2.origin = 20;
  rib.erase(entry2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  BOOST_CHECK(rib.find(entry2) == rib.end());
  BOOST_CHECK(rib.find(entry1) != rib.end());

  rib.erase(entry1);
  BOOST_CHECK_EQUAL(rib.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace rib
} // namespace nfd
