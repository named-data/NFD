/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/internal-face.hpp"
#include "mgmt/fib-manager.hpp"
#include "table/fib.hpp"


#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(MgmtInternalFace)

shared_ptr<Face>
getFace(FaceId id)
{
  return shared_ptr<Face>();
}

BOOST_AUTO_TEST_CASE(ValidPrefixRegistration)
{
  Fib fib;
  FibManager manager(fib, &getFace);
  InternalFace internal(manager);

  Name regName(manager.getRequestPrefix());
  regName.append("hello").append("world");
  Interest regInterest(regName);
  internal.sendInterest(regInterest);
}

BOOST_AUTO_TEST_CASE(InvalidPrefixRegistration)
{
  Fib fib;
  FibManager manager(fib, &getFace);
  InternalFace internal(manager);
  Interest nonRegInterest("/hello/world");
  internal.sendInterest(nonRegInterest);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
