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

#include "core/network.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestNetwork, BaseFixture)

using boost::asio::ip::address;

BOOST_AUTO_TEST_CASE(Empty)
{
  Network n;
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("192.0.2.1")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:0:3025:ccc5:eeeb:86d3")),
                    false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("0.0.0.1")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("255.255.255.255")), false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("::")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")),
                    false);
}

BOOST_AUTO_TEST_CASE(MaxRangeV4)
{
  Network n = Network::getMaxRangeV4();
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("192.0.2.1")), true);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:1:3025:ccc5:eeeb:86d3")),
                    false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("0.0.0.1")), true);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("255.255.255.255")), true);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("::")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")),
                    false);
}

BOOST_AUTO_TEST_CASE(RangeV4)
{
  Network n = boost::lexical_cast<Network>("192.0.2.0/24");
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n), "192.0.2.0 <-> 192.0.2.255");

  BOOST_CHECK_THROW(boost::lexical_cast<Network>("192.0.2.0/255"), boost::bad_lexical_cast);
  BOOST_CHECK_THROW(boost::lexical_cast<Network>("256.0.2.0/24"), boost::bad_lexical_cast);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("192.0.2.1")), true);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("192.0.2.254")), true);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("192.0.1.255")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("192.0.3.0")), false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:1:3025:ccc5:eeeb:86d3")),
                    false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("0.0.0.1")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("255.255.255.255")), false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("::")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")),
                    false);
}

BOOST_AUTO_TEST_CASE(MaxRangeV6)
{
  Network n = Network::getMaxRangeV6();
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("192.0.2.1")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:1:3025:ccc5:eeeb:86d3")),
                    true);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("0.0.0.1")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("255.255.255.255")), false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("::")), true);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")),
                    true);
}

BOOST_AUTO_TEST_CASE(RangeV6)
{
  Network n = boost::lexical_cast<Network>("2001:db8:3f9:1::/64");
  BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),
                    "2001:db8:3f9:1:: <-> 2001:db8:3f9:1:ffff:ffff:ffff:ffff");

  BOOST_CHECK_THROW(boost::lexical_cast<Network>("2001:db8:3f9:1::/129"), boost::bad_lexical_cast);
  BOOST_CHECK_THROW(boost::lexical_cast<Network>("200x:db8:3f9:1::/64"), boost::bad_lexical_cast);
  BOOST_CHECK_THROW(boost::lexical_cast<Network>("2001:db8:3f9::1::/64"), boost::bad_lexical_cast);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("192.0.2.1")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:1:3025:ccc5:eeeb:86d3")),
                    true);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:1::")),
                    true);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:1:ffff:ffff:ffff:ffff")),
                    true);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:0:ffff:ffff:ffff:ffff")),
                    false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("2001:db8:3f9:2::")),
                    false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("0.0.0.1")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("255.255.255.255")), false);

  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("::")), false);
  BOOST_CHECK_EQUAL(n.doesContain(address::from_string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")),
                    false);
}

BOOST_AUTO_TEST_CASE(Comparisons)
{
  BOOST_CHECK_EQUAL(boost::lexical_cast<Network>("192.0.2.0/24"),
                    boost::lexical_cast<Network>("192.0.2.127/24"));

  BOOST_CHECK_EQUAL(boost::lexical_cast<Network>("2001:db8:3f9:0::/64"),
                    boost::lexical_cast<Network>("2001:db8:3f9:0:ffff::/64"));

  BOOST_CHECK_NE(boost::lexical_cast<Network>("192.0.2.0/24"),
                 boost::lexical_cast<Network>("192.0.3.127/24"));

  BOOST_CHECK_NE(boost::lexical_cast<Network>("2001:db8:3f9:0::/64"),
                 boost::lexical_cast<Network>("2001:db8:3f9:1::/64"));

  BOOST_CHECK_NE(boost::lexical_cast<Network>("192.0.2.0/24"),
                 boost::lexical_cast<Network>("2001:db8:3f9:0::/64"));
}

BOOST_AUTO_TEST_CASE(IsValidCidr)
{
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/24"), true);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.1.2.3/32"), true);
  BOOST_CHECK_EQUAL(Network::isValidCidr("0.0.0.0/0"), true);
  BOOST_CHECK_EQUAL(Network::isValidCidr(""), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/24/8"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("/192.0.0.0/24"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/+24"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/-24"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/ 24"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/24a"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/0x42"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/24.42"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/foo"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/33"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/999999999999999"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0/"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("192.0.0.0"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("foo/4"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("foo/"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("foo"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("256.0.256.0/24"), false);

  BOOST_CHECK_EQUAL(Network::isValidCidr("::1"), false);
  BOOST_CHECK_EQUAL(Network::isValidCidr("::1/128"), true);
}

BOOST_AUTO_TEST_SUITE_END() // TestNetwork

} // namespace tests
} // namespace nfd
