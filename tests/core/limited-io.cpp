/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "limited-io.hpp"

namespace nfd {

const int LimitedIo::UNLIMITED_OPS = std::numeric_limits<int>::max();
const time::Duration LimitedIo::UNLIMITED_TIME = time::nanoseconds(-1);

LimitedIo::LimitedIo()
  : m_isRunning(false)
  , m_nOpsRemaining(0)
{
  resetGlobalIoService();
}

LimitedIo::StopReason
LimitedIo::run(int nOpsLimit, time::Duration nTimeLimit)
{
  BOOST_ASSERT(!m_isRunning);
  m_isRunning = true;
  
  m_reason = NO_WORK;
  m_nOpsRemaining = nOpsLimit;
  if (nTimeLimit != UNLIMITED_TIME) {
    m_timeout = scheduler::schedule(nTimeLimit, bind(&LimitedIo::afterTimeout, this));
  }
  
  getGlobalIoService().run();
  
  getGlobalIoService().reset();
  scheduler::cancel(m_timeout);
  m_isRunning = false;
  return m_reason;
}

void
LimitedIo::afterOp()
{
  --m_nOpsRemaining;
  if (m_nOpsRemaining <= 0) {
    m_reason = EXCEED_OPS;
    getGlobalIoService().stop();
  }
}

void
LimitedIo::afterTimeout()
{
  m_reason = EXCEED_TIME;
  getGlobalIoService().stop();
}

} // namespace nfd
