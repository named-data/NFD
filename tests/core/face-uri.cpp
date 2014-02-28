/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "core/face-uri.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreFaceUri, BaseFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  BOOST_CHECK_NO_THROW(FaceUri("udp://hostname:6363"));
  BOOST_CHECK_THROW(FaceUri("udp//hostname:6363"), FaceUri::Error);
  BOOST_CHECK_THROW(FaceUri("udp://hostname:port"), FaceUri::Error);

  FaceUri uri;
  BOOST_CHECK_EQUAL(uri.parse("udp//hostname:6363"), false);
  
  BOOST_CHECK(uri.parse("udp://hostname:80"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "udp");
  BOOST_CHECK_EQUAL(uri.getDomain(), "hostname");
  BOOST_CHECK_EQUAL(uri.getPort(), "80");

  BOOST_CHECK(uri.parse("udp4://192.0.2.1:20"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "udp4");
  BOOST_CHECK_EQUAL(uri.getDomain(), "192.0.2.1");
  BOOST_CHECK_EQUAL(uri.getPort(), "20");

  BOOST_CHECK(uri.parse("udp6://[2001:db8:3f9:0::1]:6363"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "udp6");
  BOOST_CHECK_EQUAL(uri.getDomain(), "2001:db8:3f9:0::1");
  BOOST_CHECK_EQUAL(uri.getPort(), "6363");
  
  BOOST_CHECK(uri.parse("udp6://[2001:db8:3f9:0:3025:ccc5:eeeb:86d3]:6363"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "udp6");
  BOOST_CHECK_EQUAL(uri.getDomain(), "2001:db8:3f9:0:3025:ccc5:eeeb:86d3");
  BOOST_CHECK_EQUAL(uri.getPort(), "6363");

  BOOST_CHECK(uri.parse("tcp://random.host.name"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "tcp");
  BOOST_CHECK_EQUAL(uri.getDomain(), "random.host.name");
  BOOST_CHECK_EQUAL(uri.getPort(), "");

  BOOST_CHECK_EQUAL(uri.parse("tcp://192.0.2.1:"), false);
  BOOST_CHECK_EQUAL(uri.parse("tcp://[::zzzz]"), false);
  BOOST_CHECK_EQUAL(uri.parse("udp6://[2001:db8:3f9:0:3025:ccc5:eeeb:86dg]:6363"), false);  
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
