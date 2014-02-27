/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

// #include "unit-under-test.hpp"
// Unit being tested MUST be included first, to ensure header compiles on its own.

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {
// Unit tests SHOULD go inside nfd::tests namespace.

// Test suite SHOULD use BaseFixture or a subclass of it.
BOOST_FIXTURE_TEST_SUITE(TestSkeleton, BaseFixture)

BOOST_AUTO_TEST_CASE(Test1)
{
  int i = 0;
  /**
   * For reference of available Boost.Test macros, @see http://www.boost.org/doc/libs/1_55_0/libs/test/doc/html/utf/testing-tools/reference.html
   */

  BOOST_REQUIRE_NO_THROW(i = 1);
  BOOST_REQUIRE_EQUAL(i, 1);
}

// Custom fixture SHOULD derive from BaseFixture.
class Test2Fixture : protected BaseFixture
{
};

BOOST_FIXTURE_TEST_CASE(Test2, Test2Fixture)
{
  // g_io is a shorthand of getGlobalIoService()
  // resetGlobalIoService() is automatically called after each test case
  g_io.run();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
