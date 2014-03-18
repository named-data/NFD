/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "rib.hpp"

#include <boost/test/unit_test.hpp>

namespace ndn {
namespace nrd {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestRib)

BOOST_AUTO_TEST_CASE(Basic)
{
  Rib rib;

  PrefixRegOptions options1;
  options1.setName("/hello/world");
  options1.setFlags(tlv::nrd::NDN_FORW_CHILD_INHERIT | tlv::nrd::NDN_FORW_CAPTURE);
  options1.setCost(10);
  options1.setExpirationPeriod(time::milliseconds(1500));

  PrefixRegOptions options2;
  options2.setName("/hello/world");
  options2.setFlags(tlv::nrd::NDN_FORW_CHILD_INHERIT);
  options2.setExpirationPeriod(time::seconds(0));

  rib.insert(options1);
  BOOST_CHECK_EQUAL(rib.size(), 1);
  
  rib.insert(options2);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  options2.setName("/foo/bar");
  rib.insert(options2);
  BOOST_CHECK_EQUAL(rib.size(), 2);

  rib.erase(options2);
  BOOST_CHECK_EQUAL(rib.size(), 1);

  BOOST_CHECK(rib.find(options2) == rib.end());
  BOOST_CHECK(rib.find(options1) != rib.end());

  rib.erase(options1);
  BOOST_CHECK(rib.empty());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nrd
} // namespace ndn
