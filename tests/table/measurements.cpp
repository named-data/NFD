/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/measurements.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(TableMeasurements)

BOOST_AUTO_TEST_CASE(Get_Parent)
{
  Name name0;
  Name nameA ("ndn:/A");
  Name nameAB("ndn:/A/B");

  boost::asio::io_service ioService;
  Measurements measurements(ioService);

  shared_ptr<measurements::Entry> entryAB = measurements.get(nameAB);
  BOOST_REQUIRE(static_cast<bool>(entryAB));
  BOOST_CHECK_EQUAL(entryAB->getName(), nameAB);

  shared_ptr<measurements::Entry> entry0 = measurements.get(name0);
  BOOST_REQUIRE(static_cast<bool>(entry0));

  shared_ptr<measurements::Entry> entryA = measurements.getParent(entryAB);
  BOOST_REQUIRE(static_cast<bool>(entryA));
  BOOST_CHECK_EQUAL(entryA->getName(), nameA);

  shared_ptr<measurements::Entry> entry0c = measurements.getParent(entryA);
  BOOST_CHECK_EQUAL(entry0, entry0c);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
