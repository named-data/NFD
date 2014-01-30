/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/internal-face.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(MgmtInternalFace)

BOOST_AUTO_TEST_CASE(ValidPrefixRegistration)
{
  InternalFace internal;
  Interest regInterest("/localhost/nfd/prefixreg/hello/world");
  internal.sendInterest(regInterest);
}

BOOST_AUTO_TEST_CASE(InvalidPrefixRegistration)
{
  InternalFace internal;
  Interest nonRegInterest("/hello/world");
  internal.sendInterest(nonRegInterest);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
