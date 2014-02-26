/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "global-io.hpp"

namespace nfd {

namespace scheduler {
// defined in scheduler.cpp
void
resetGlobalScheduler();
} // namespace scheduler

static shared_ptr<boost::asio::io_service> g_ioService;

boost::asio::io_service&
getGlobalIoService()
{
  if (!static_cast<bool>(g_ioService)) {
    g_ioService = make_shared<boost::asio::io_service>();
  }
  return *g_ioService;
}

void
resetGlobalIoService()
{
  scheduler::resetGlobalScheduler();
  g_ioService.reset();
}

} // namespace nfd
