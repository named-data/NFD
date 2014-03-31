/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "scheduler.hpp"
#include "global-io.hpp"

namespace nfd {
namespace scheduler {

static shared_ptr<Scheduler> g_scheduler;

inline Scheduler&
getGlobalScheduler()
{
  if (!static_cast<bool>(g_scheduler)) {
    g_scheduler = make_shared<Scheduler>(boost::ref(getGlobalIoService()));
  }
  return *g_scheduler;
}

EventId
schedule(const time::nanoseconds& after, const Scheduler::Event& event)
{
  return getGlobalScheduler().scheduleEvent(after, event);
}

void
cancel(const EventId& eventId)
{
  getGlobalScheduler().cancelEvent(eventId);
}

void
resetGlobalScheduler()
{
  g_scheduler.reset();
}

} // namespace scheduler
} // namespace nfd
