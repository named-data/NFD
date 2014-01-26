/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/face.hpp"
#include "dummy-face.hpp"

#include <boost/test/unit_test.hpp>

namespace ndn {

BOOST_AUTO_TEST_SUITE(FaceFace)

BOOST_AUTO_TEST_CASE(Description)
{
  DummyFace face(1);
  face.setDescription("3pFsKrvWr");
  BOOST_CHECK_EQUAL(face.getDescription(), "3pFsKrvWr");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace ndn
