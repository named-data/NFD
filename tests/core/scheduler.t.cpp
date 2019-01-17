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

#include "core/scheduler.hpp"

#include "tests/test-common.hpp"

#include <boost/thread.hpp>

namespace nfd {
namespace scheduler {
// defined in scheduler.cpp
Scheduler&
getGlobalScheduler();
} // namespace scheduler

namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestScheduler, BaseFixture)

BOOST_AUTO_TEST_CASE(ScheduleCancel)
{
  int count1 = 0, count2 = 0, count3 = 0;

  scheduler::schedule(500_ms, [&] {
    BOOST_CHECK_EQUAL(count3, 1);
    ++count1;
  });

  scheduler::EventId eid = scheduler::schedule(1_s, [&] { ++count2; });
  scheduler::cancel(eid);

  scheduler::schedule(250_ms, [&] {
    BOOST_CHECK_EQUAL(count1, 0);
    ++count3;
  });

  {
    scheduler::ScopedEventId se = scheduler::schedule(50_ms, [&] { ++count2; });
  } // se goes out of scope, canceling the event

  g_io.run();

  BOOST_CHECK_EQUAL(count1, 1);
  BOOST_CHECK_EQUAL(count2, 0);
  BOOST_CHECK_EQUAL(count3, 1);
}

BOOST_AUTO_TEST_CASE(ThreadLocalScheduler)
{
  scheduler::Scheduler* s1 = &scheduler::getGlobalScheduler();
  scheduler::Scheduler* s2 = nullptr;
  boost::thread t([&s2] { s2 = &scheduler::getGlobalScheduler(); });
  t.join();

  BOOST_CHECK(s1 != nullptr);
  BOOST_CHECK(s2 != nullptr);
  BOOST_CHECK(s1 != s2);
}

BOOST_AUTO_TEST_SUITE_END() // TestScheduler

} // namespace tests
} // namespace nfd
