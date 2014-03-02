/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "core/version.hpp"
#include "core/logger.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreVersion, BaseFixture)

NFD_LOG_INIT("VersionTest");

BOOST_AUTO_TEST_CASE(Version)
{
  NFD_LOG_INFO("NFD_VERSION " << NFD_VERSION);
  
  BOOST_CHECK_EQUAL(NFD_VERSION, NFD_VERSION_MAJOR * 1000000 +
                                 NFD_VERSION_MINOR * 1000 +
                                 NFD_VERSION_PATCH);
}

BOOST_AUTO_TEST_CASE(VersionString)
{
  NFD_LOG_INFO("NFD_VERSION_STRING " << NFD_VERSION_STRING);
  
  BOOST_STATIC_ASSERT(NFD_VERSION_MAJOR < 1000);
  char buf[12];
  snprintf(buf, sizeof(buf), "%d.%d.%d", NFD_VERSION_MAJOR, NFD_VERSION_MINOR, NFD_VERSION_PATCH);
  
  BOOST_CHECK_EQUAL(std::string(NFD_VERSION_STRING), std::string(buf));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
