/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "core/map-value-iterator.hpp"
#include <boost/concept_check.hpp>

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreMapValueIterator, BaseFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  typedef std::map<char, int> CharIntMap;
  typedef MapValueIterator<CharIntMap> CharIntMapValueIterator;
  BOOST_CONCEPT_ASSERT((boost::ForwardIterator<CharIntMapValueIterator>));

  CharIntMap map;
  map['a'] = 1918;
  map['b'] = 2675;
  map['c'] = 7783;
  map['d'] = 2053;

  CharIntMapValueIterator begin(map.begin());
  CharIntMapValueIterator end  (map.end  ());

  int expected[] = { 1918, 2675, 7783, 2053 };
  BOOST_CHECK_EQUAL_COLLECTIONS(begin, end, expected, expected + 4);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
