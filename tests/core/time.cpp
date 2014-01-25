/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "core/time.hpp"

#include <boost/test/unit_test.hpp>

namespace ndn {

BOOST_AUTO_TEST_SUITE(CoreTime)

BOOST_AUTO_TEST_CASE(Now)
{
  time::Point p1 = time::now();
  time::Point p2 = time::now();
  BOOST_CHECK_LE(p1, p2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace ndn
