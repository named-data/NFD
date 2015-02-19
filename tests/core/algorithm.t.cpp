/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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
 */

#include "core/algorithm.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestAlgorithm, BaseFixture)

BOOST_AUTO_TEST_CASE(FindLastIf)
{
  std::vector<int> vec{1, 2, 3, 4, 5, 6, 7, 8, 9};

  int hit1 = 0;
  std::vector<int>::const_iterator found1 = find_last_if(vec.begin(), vec.end(),
      [&hit1] (int n) -> bool {
        ++hit1;
        return n % 2 == 0;
      });
  BOOST_REQUIRE(found1 != vec.end());
  BOOST_CHECK_EQUAL(*found1, 8);
  BOOST_CHECK_LE(hit1, vec.size());

  int hit2 = 0;
  std::vector<int>::const_iterator found2 = find_last_if(vec.begin(), vec.end(),
      [&hit2] (int n) -> bool {
        ++hit2;
        return n < 0;
      });
  BOOST_CHECK(found2 == vec.end());
  BOOST_CHECK_LE(hit2, vec.size());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
