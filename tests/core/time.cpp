/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "core/time.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(CoreTime)

BOOST_AUTO_TEST_CASE(Now)
{
  time::Point p1 = time::now();
  time::Point p2 = time::now();
  BOOST_CHECK_LE(p1, p2);
}

BOOST_AUTO_TEST_CASE(Operations)
{
  // helpers
  BOOST_CHECK_GT(time::seconds(1), time::milliseconds(1));
  BOOST_CHECK_GT(time::milliseconds(1), time::microseconds(1));
  BOOST_CHECK_GT(time::microseconds(1), time::nanoseconds(1));
  
  // duration operations + helpers
  BOOST_CHECK_EQUAL(time::seconds(8) + time::microseconds(101), time::nanoseconds(8000101000));
  BOOST_CHECK_EQUAL(time::seconds(7) - time::milliseconds(234), time::microseconds(6766000));

  // point operations
  time::Point p1 = time::now();
  time::Point p2 = p1 + time::milliseconds(10000);
  time::Point p3 = p2 - time::microseconds(5000000);
  BOOST_CHECK_LE(p1, p2);
  BOOST_CHECK_LE(p1, p3);
  BOOST_CHECK_LE(p3, p2);
  
  BOOST_CHECK_EQUAL(p2 - p1, time::seconds(10));
  BOOST_CHECK_EQUAL(p3 - p1, time::seconds(5));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
