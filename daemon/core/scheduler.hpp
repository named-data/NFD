/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_SCHEDULER_HPP
#define NFD_CORE_SCHEDULER_HPP

#include "common.hpp"
#include "monotonic_deadline_timer.hpp"

namespace nfd {

struct EventIdImpl; ///< \brief Private storage of information about the event
/**
 * \brief Opaque type (shared_ptr) representing ID of the scheduled event
 */
typedef shared_ptr<EventIdImpl> EventId;

/**
 * \brief Generic scheduler
 */
class Scheduler
{
public:
  typedef function<void()> Event;

  Scheduler(boost::asio::io_service& ioService);

  /**
   * \brief Schedule one time event after the specified delay
   * \returns EventId that can be used to cancel the scheduled event
   */
  EventId
  scheduleEvent(const time::Duration& after, const Event& event);

  /**
   * \brief Schedule periodic event that should be fired every specified period.
   *        First event will be fired after the specified delay.
   * \returns EventId that can be used to cancel the scheduled event
   */
  EventId
  schedulePeriodicEvent(const time::Duration& after,
                        const time::Duration& period,
                        const Event& event);
  
  /**
   * \brief Cancel scheduled event
   */
  void
  cancelEvent(const EventId& eventId);

private:
  void
  onEvent(const boost::system::error_code& code);
  
private:
  boost::asio::io_service& m_ioService;

  struct EventInfo
  {
    EventInfo(const time::Duration& after,
              const time::Duration& period,
              const Event& event);
    EventInfo(const time::Point& when, const EventInfo& previousEvent);

    bool
    operator <=(const EventInfo& other) const
    {
      return this->m_scheduledTime <= other.m_scheduledTime;
    }

    bool
    operator <(const EventInfo& other) const
    {
      return this->m_scheduledTime < other.m_scheduledTime;
    }

    time::Duration
    expiresFromNow() const;
    
    time::Point m_scheduledTime;
    time::Duration m_period;
    Event m_event;
    mutable EventId m_eventId;
  };

  typedef std::multiset<EventInfo> EventQueue;
  friend struct EventIdImpl;

  EventQueue m_events;
  EventQueue::iterator m_scheduledEvent;
  boost::asio::monotonic_deadline_timer m_deadlineTimer;

  bool m_isEventExecuting;
};

} // namespace nfd

#endif // NFD_CORE_SCHEDULER_HPP
