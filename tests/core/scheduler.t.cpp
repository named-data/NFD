/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

using scheduler::EventId;
using scheduler::ScopedEventId;

BOOST_FIXTURE_TEST_SUITE(TestScheduler, BaseFixture)

class SchedulerFixture : protected BaseFixture
{
public:
  SchedulerFixture()
    : count1(0)
    , count2(0)
    , count3(0)
  {
  }

  void
  event1()
  {
    BOOST_CHECK_EQUAL(count3, 1);
    ++count1;
  }

  void
  event2()
  {
    ++count2;
  }

  void
  event3()
  {
    BOOST_CHECK_EQUAL(count1, 0);
    ++count3;
  }

public:
  int count1;
  int count2;
  int count3;
};

BOOST_FIXTURE_TEST_CASE(Events, SchedulerFixture)
{
  scheduler::schedule(time::milliseconds(500), bind(&SchedulerFixture::event1, this));

  EventId i = scheduler::schedule(time::seconds(1), bind(&SchedulerFixture::event2, this));
  scheduler::cancel(i);

  scheduler::schedule(time::milliseconds(250), bind(&SchedulerFixture::event3, this));

  i = scheduler::schedule(time::milliseconds(50), bind(&SchedulerFixture::event2, this));
  scheduler::cancel(i);

  g_io.run();

  BOOST_CHECK_EQUAL(count1, 1);
  BOOST_CHECK_EQUAL(count2, 0);
  BOOST_CHECK_EQUAL(count3, 1);
}

BOOST_AUTO_TEST_CASE(CancelEmptyEvent)
{
  EventId i;
  scheduler::cancel(i);

  // Trivial check to avoid "test case did not check any assertions" message from Boost.Test
  BOOST_CHECK(true);
}

class SelfCancelFixture : protected BaseFixture
{
public:
  void
  cancelSelf() const
  {
    scheduler::cancel(m_selfEventId);
  }

public:
  EventId m_selfEventId;
};

BOOST_FIXTURE_TEST_CASE(SelfCancel, SelfCancelFixture)
{
  m_selfEventId = scheduler::schedule(time::milliseconds(100),
                                      bind(&SelfCancelFixture::cancelSelf, this));

  BOOST_REQUIRE_NO_THROW(g_io.run());
}

BOOST_FIXTURE_TEST_CASE(ScopedEventIdDestruct, UnitTestTimeFixture)
{
  int hit = 0;
  {
    ScopedEventId se = scheduler::schedule(time::milliseconds(10), [&] { ++hit; });
  } // se goes out of scope
  this->advanceClocks(time::milliseconds(1), 15);
  BOOST_CHECK_EQUAL(hit, 0);
}

BOOST_FIXTURE_TEST_CASE(ScopedEventIdAssign, UnitTestTimeFixture)
{
  int hit1 = 0, hit2 = 0;
  ScopedEventId se1 = scheduler::schedule(time::milliseconds(10), [&] { ++hit1; });
  se1 = scheduler::schedule(time::milliseconds(10), [&] { ++hit2; });
  this->advanceClocks(time::milliseconds(1), 15);
  BOOST_CHECK_EQUAL(hit1, 0);
  BOOST_CHECK_EQUAL(hit2, 1);
}

BOOST_FIXTURE_TEST_CASE(ScopedEventIdRelease, UnitTestTimeFixture)
{
  int hit = 0;
  {
    ScopedEventId se = scheduler::schedule(time::milliseconds(10), [&] { ++hit; });
    se.release();
  } // se goes out of scope
  this->advanceClocks(time::milliseconds(1), 15);
  BOOST_CHECK_EQUAL(hit, 1);
}

BOOST_FIXTURE_TEST_CASE(ScopedEventIdMove, UnitTestTimeFixture)
{
  int hit = 0;
  unique_ptr<scheduler::ScopedEventId> se2;
  {
    ScopedEventId se = scheduler::schedule(time::milliseconds(10), [&] { ++hit; });
    se2.reset(new ScopedEventId(std::move(se)));
  } // se goes out of scope
  this->advanceClocks(time::milliseconds(1), 15);
  BOOST_CHECK_EQUAL(hit, 1);
}

BOOST_AUTO_TEST_CASE(ThreadLocalScheduler)
{
  scheduler::Scheduler* s1 = &scheduler::getGlobalScheduler();
  scheduler::Scheduler* s2 = nullptr;
  boost::thread t([&s2] {
      s2 = &scheduler::getGlobalScheduler();
    });

  t.join();

  BOOST_CHECK(s1 != nullptr);
  BOOST_CHECK(s2 != nullptr);
  BOOST_CHECK(s1 != s2);
}

BOOST_AUTO_TEST_SUITE_END() // TestScheduler

} // namespace tests
} // namespace nfd
