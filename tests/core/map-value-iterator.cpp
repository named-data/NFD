/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

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
