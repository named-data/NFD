/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_SCHEDULER_HPP
#define NFD_CORE_SCHEDULER_HPP

#include "common.hpp"
#include <ndn-cpp-dev/util/scheduler.hpp>

namespace nfd {
namespace scheduler {

using ndn::Scheduler;

/** \class EventId
 *  \brief Opaque type (shared_ptr) representing ID of a scheduled event
 */
using ndn::EventId;

/** \brief schedule an event
 */
EventId
schedule(const time::nanoseconds& after, const Scheduler::Event& event);

/** \brief cancel a scheduled event
 */
void
cancel(const EventId& eventId);

} // namespace scheduler

using scheduler::EventId;

} // namespace nfd

#endif // NFD_CORE_SCHEDULER_HPP
