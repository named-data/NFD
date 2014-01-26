/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

/**
 * This code is based on https://svn.boost.org/trac/boost/attachment/ticket/3504/MonotonicDeadlineTimer.h
 */

#ifndef NFD_CORE_MONOTONIC_DEADLINE_TIMER_HPP
#define NFD_CORE_MONOTONIC_DEADLINE_TIMER_HPP

#include "time.hpp"

namespace boost {
namespace asio {

template <> 
struct time_traits<ndn::time::monotonic_clock>
{
  typedef ndn::time::Point time_type;
  typedef ndn::time::Duration duration_type;

  static time_type
  now()
  {
    return ndn::time::now();
  }

  static time_type
  add(const time_type& time, const duration_type& duration) 
  {
    return time + duration;
  }

  static duration_type
  subtract(const time_type& timeLhs, const time_type& timeRhs)
  {
    return timeLhs - timeRhs;
  }

  static bool
  less_than(const time_type& timeLhs, const time_type& timeRhs)
  {
    return timeLhs < timeRhs;
  }

  static boost::posix_time::time_duration
  to_posix_duration(const duration_type& duration)
  {
    return boost::posix_time::microseconds(duration/1000);
  }
};

typedef basic_deadline_timer<ndn::time::monotonic_clock> monotonic_deadline_timer; 

} // namespace asio
} // namespace boost

#endif // NFD_CORE_MONOTONIC_DEADLINE_TIMER_HPP
