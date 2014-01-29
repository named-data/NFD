/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/cs.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(TableCs)

BOOST_AUTO_TEST_CASE(FakeInsertFind)
{
  Cs cs;
  
  Data data;
  BOOST_CHECK_EQUAL(cs.insert(data), false);
  
  Interest interest;
  BOOST_CHECK_EQUAL(static_cast<bool>(cs.find(interest)), false);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
