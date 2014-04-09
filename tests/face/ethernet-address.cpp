/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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

#include "face/ethernet.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceEthernetAddress, BaseFixture)

BOOST_AUTO_TEST_CASE(Checks)
{
  BOOST_CHECK(ethernet::Address().isNull());
  BOOST_CHECK(ethernet::getBroadcastAddress().isBroadcast());
  BOOST_CHECK(ethernet::getDefaultMulticastAddress().isMulticast());
}

BOOST_AUTO_TEST_CASE(ToString)
{
  BOOST_CHECK_EQUAL(ethernet::Address().toString('-'),
                    "00-00-00-00-00-00");
  BOOST_CHECK_EQUAL(ethernet::getBroadcastAddress().toString(),
                    "ff:ff:ff:ff:ff:ff");
  BOOST_CHECK_EQUAL(ethernet::Address(0x01, 0x23, 0x45, 0x67, 0x89, 0xAB).toString('-'),
                    "01-23-45-67-89-ab");
  BOOST_CHECK_EQUAL(ethernet::Address(0x01, 0x23, 0x45, 0x67, 0x89, 0xAB).toString(),
                    "01:23:45:67:89:ab");
}

BOOST_AUTO_TEST_CASE(FromString)
{
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("0:0:0:0:0:0"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("ff-ff-ff-ff-ff-ff"),
                    ethernet::getBroadcastAddress());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("de:ad:be:ef:1:2"),
                    ethernet::Address(0xde, 0xad, 0xbe, 0xef, 0x01, 0x02));

  // malformed inputs
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("01.23.45.67.89.ab"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("01:23:45 :67:89:ab"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("01:23:45:67:89::1"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("01-23-45-67-89"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("01:23:45:67:89:ab:cd"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("qw-er-ty-12-34-56"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("this-is-not-an-ethernet-address"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString("foobar"),
                    ethernet::Address());
  BOOST_CHECK_EQUAL(ethernet::Address::fromString(""),
                    ethernet::Address());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
