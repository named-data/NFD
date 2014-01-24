/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/cs.hpp"
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
namespace ndn {

BOOST_AUTO_TEST_SUITE(TableCs)

BOOST_AUTO_TEST_CASE(FakeInsertFind)
{
  Cs cs;
  
  boost::shared_ptr<Data> data = boost::make_shared<Data>();
  BOOST_CHECK_EQUAL(cs.insert(data), false);
  
  Interest interest;
  BOOST_CHECK_EQUAL(cs.find(interest).get(), static_cast<cs::Entry*>(0));
}

BOOST_AUTO_TEST_SUITE_END()
};//namespace ndn
