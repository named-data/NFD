/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "scheduler.hpp"

namespace nfd {
namespace scheduler {

static shared_ptr<Scheduler> g_scheduler;

Scheduler&
getGlobalScheduler()
{
  if (!static_cast<bool>(g_scheduler)) {
    g_scheduler = make_shared<Scheduler>(boost::ref(getGlobalIoService()));
  }
  return *g_scheduler;
}

void
resetGlobalScheduler()
{
  g_scheduler.reset();
}

} // namespace scheduler
} // namespace nfd
