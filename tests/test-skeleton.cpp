/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TestSkeleton)

BOOST_AUTO_TEST_CASE (Test1)
{
  int i = 0;
  /**
   * For reference of available Boost.Test macros, @see http://www.boost.org/doc/libs/1_55_0/libs/test/doc/html/utf/testing-tools/reference.html
   */

  BOOST_REQUIRE_NO_THROW(i = 1);
  BOOST_REQUIRE_EQUAL(i, 1);
}

BOOST_AUTO_TEST_SUITE_END()
