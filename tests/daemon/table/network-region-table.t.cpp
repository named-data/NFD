/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "table/network-region-table.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestNetworkRegionTable, BaseFixture)

BOOST_AUTO_TEST_CASE(InProducerRegion)
{
  DelegationList fh{{10, "/telia/terabits"}, {20, "/ucla/cs"}};

  NetworkRegionTable nrt1;
  nrt1.insert("/verizon");
  BOOST_CHECK_EQUAL(nrt1.isInProducerRegion(fh), false);

  NetworkRegionTable nrt2;
  nrt2.insert("/ucla");
  BOOST_CHECK_EQUAL(nrt2.isInProducerRegion(fh), false);

  NetworkRegionTable nrt3;
  nrt3.insert("/ucla/cs");
  BOOST_CHECK_EQUAL(nrt3.isInProducerRegion(fh), true);

  NetworkRegionTable nrt4;
  nrt4.insert("/ucla/cs/software");
  nrt4.insert("/ucla/cs/irl");
  BOOST_CHECK_EQUAL(nrt4.isInProducerRegion(fh), true);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
