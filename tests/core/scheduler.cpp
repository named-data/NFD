/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "core/scheduler.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreScheduler, BaseFixture)

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
}

class SelfCancelFixture : protected BaseFixture
{
public:
  void
  cancelSelf()
  {
    scheduler::cancel(m_selfEventId);
  }

  EventId m_selfEventId;
};

BOOST_FIXTURE_TEST_CASE(SelfCancel, SelfCancelFixture)
{
  m_selfEventId = scheduler::schedule(time::milliseconds(100),
                                      bind(&SelfCancelFixture::cancelSelf, this));

  BOOST_REQUIRE_NO_THROW(g_io.run());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
