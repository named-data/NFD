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

  PrefixRegOptions options1;
  options1.setName("/hello/world");
  options1.setFlags(tlv::nrd::NDN_FORW_CHILD_INHERIT | tlv::nrd::NDN_FORW_CAPTURE);
  options1.setCost(10);
  options1.setExpirationPeriod(time::milliseconds(1500));
  options1.setFaceId(1);

  rib.insert(options1);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  PrefixRegOptions options2;
  options2.setName("/hello/world");
  options2.setFlags(tlv::nrd::NDN_FORW_CHILD_INHERIT);
  options2.setExpirationPeriod(time::seconds(0));
  options2.setFaceId(1);
  options2.setCost(100);

  rib.insert(options2);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  options2.setFaceId(2);
  rib.insert(options2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  options2.setName("/foo/bar");
  rib.insert(options2);
  BOOST_CHECK_EQUAL(rib.size(), 3);

  rib.erase(options2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  options2.setName("/hello/world");
  rib.erase(options2);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  BOOST_CHECK(rib.find(options2) == rib.end());
  BOOST_CHECK(rib.find(options1) != rib.end());

  rib.erase(options1);
  BOOST_CHECK(rib.empty());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace rib
} // namespace nfd
