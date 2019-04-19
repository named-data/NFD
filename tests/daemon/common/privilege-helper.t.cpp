/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "common/privilege-helper.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestPrivilegeHelper)

BOOST_AUTO_TEST_CASE(DropRaise)
{
#ifdef HAVE_PRIVILEGE_DROP_AND_ELEVATE
  SKIP_IF_NOT_SUPERUSER();

  // The following assumes that daemon:daemon is present on the test system
  PrivilegeHelper::initialize("daemon", "daemon");
  BOOST_CHECK_EQUAL(::geteuid(), 0);
  BOOST_CHECK_EQUAL(::getegid(), 0);

  PrivilegeHelper::drop();
  BOOST_CHECK_NE(::geteuid(), 0);
  BOOST_CHECK_NE(::getegid(), 0);

  PrivilegeHelper::runElevated([] {
    BOOST_CHECK_EQUAL(::geteuid(), 0);
    BOOST_CHECK_EQUAL(::getegid(), 0);
  });
  BOOST_CHECK_NE(::geteuid(), 0);
  BOOST_CHECK_NE(::getegid(), 0);

  BOOST_CHECK_THROW(PrivilegeHelper::runElevated(std::function<void()>{}),
                    std::bad_function_call);
  BOOST_CHECK_NE(::geteuid(), 0);
  BOOST_CHECK_NE(::getegid(), 0);

  PrivilegeHelper::raise();
  BOOST_CHECK_EQUAL(::geteuid(), 0);
  BOOST_CHECK_EQUAL(::getegid(), 0);

#else
  BOOST_TEST_MESSAGE("Dropping/raising privileges not supported on this platform, skipping");
#endif // HAVE_PRIVILEGE_DROP_AND_ELEVATE
}

BOOST_AUTO_TEST_SUITE_END() // TestPrivilegeHelper

} // namespace tests
} // namespace nfd
