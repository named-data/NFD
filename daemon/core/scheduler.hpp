/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_SCHEDULER_HPP
#define NFD_CORE_SCHEDULER_HPP

#include "global-io.hpp"
#include <ndn-cpp-dev/util/scheduler.hpp>

namespace nfd {
namespace scheduler {

using ndn::Scheduler;

/** \class EventId
 *  \brief Opaque type (shared_ptr) representing ID of a scheduled event
 */
using ndn::EventId;

} // namespace scheduler

// TODO delete this after transition
using scheduler::Scheduler;

using scheduler::EventId;

namespace scheduler {

// TODO delete this after transition
Scheduler&
getGlobalScheduler();

inline EventId
schedule(const time::nanoseconds& after, const Scheduler::Event& event)
{
  return getGlobalScheduler().scheduleEvent(after, event);
}

inline void
cancel(const EventId& eventId)
{
  getGlobalScheduler().cancelEvent(eventId);
}

} // namespace scheduler
} // namespace nfd

#endif // NFD_CORE_SCHEDULER_HPP
