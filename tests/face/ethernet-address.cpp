/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

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
  BOOST_CHECK_EQUAL(ethernet::Address().toString(),
                    "00-00-00-00-00-00");
  BOOST_CHECK_EQUAL(ethernet::getBroadcastAddress().toString(':'),
                    "ff:ff:ff:ff:ff:ff");
  BOOST_CHECK_EQUAL(ethernet::Address(0x01, 0x23, 0x45, 0x67, 0x89, 0xAB).toString(),
                    "01-23-45-67-89-ab");
  BOOST_CHECK_EQUAL(ethernet::Address(0x01, 0x23, 0x45, 0x67, 0x89, 0xAB).toString(':'),
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
