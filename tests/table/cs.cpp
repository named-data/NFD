/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/cs.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TableCs, BaseFixture)

BOOST_AUTO_TEST_CASE(FakeInsertFind)
{
  Cs cs;
  
  Data data;
  BOOST_CHECK_EQUAL(cs.insert(data), false);
  
  Interest interest;
  BOOST_CHECK_EQUAL(static_cast<bool>(cs.find(interest)), false);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
