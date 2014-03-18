/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "limited-io.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("LimitedIo");

const int LimitedIo::UNLIMITED_OPS = std::numeric_limits<int>::max();
const time::nanoseconds LimitedIo::UNLIMITED_TIME = time::nanoseconds::min();

LimitedIo::LimitedIo()
  : m_isRunning(false)
  , m_nOpsRemaining(0)
{
}

LimitedIo::StopReason
LimitedIo::run(int nOpsLimit, const time::nanoseconds& nTimeLimit)
{
  BOOST_ASSERT(!m_isRunning);
  m_isRunning = true;
  
  m_reason = NO_WORK;
  m_nOpsRemaining = nOpsLimit;
  if (nTimeLimit >= time::nanoseconds::zero()) {
    m_timeout = scheduler::schedule(nTimeLimit, bind(&LimitedIo::afterTimeout, this));
  }
  
  try {
    getGlobalIoService().run();
  }
  catch (std::exception& ex) {
    m_reason = EXCEPTION;
    NFD_LOG_ERROR("g_io.run() exception: " << ex.what());
    m_lastException = ex;
  }
  
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

const std::exception&
LimitedIo::getLastException() const
{
  return m_lastException;
}

} // namespace tests
} // namespace nfd
