/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "core/common.hpp"

#include "tests/test-common.hpp"

namespace nfd::tests {

BOOST_AUTO_TEST_SUITE(TestNdebug)

BOOST_AUTO_TEST_CASE(Assert)
{
  BOOST_TEST(BOOST_IS_DEFINED(BOOST_ASSERT_IS_VOID) == BOOST_IS_DEFINED(NDEBUG));

#ifdef NDEBUG
  // in release builds, assertion shouldn't execute
  BOOST_ASSERT(false);
  BOOST_VERIFY(false);
#endif
}

BOOST_AUTO_TEST_CASE(SideEffect)
{
  int a = 1;
  BOOST_ASSERT((a = 2) > 0);
#ifdef NDEBUG
  BOOST_TEST(a == 1);
#else
  BOOST_TEST(a == 2);
#endif
}

BOOST_AUTO_TEST_SUITE_END() // TestNdebug

} // namespace nfd::tests
