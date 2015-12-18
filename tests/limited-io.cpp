/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "limited-io.hpp"
#include "core/extended-error-message.hpp"
#include "core/global-io.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("LimitedIo");

const int LimitedIo::UNLIMITED_OPS = std::numeric_limits<int>::max();
const time::nanoseconds LimitedIo::UNLIMITED_TIME = time::nanoseconds::min();

LimitedIo::LimitedIo(UnitTestTimeFixture* uttf)
  : m_uttf(uttf)
  , m_nOpsRemaining(0)
  , m_isRunning(false)
{
}

LimitedIo::StopReason
LimitedIo::run(int nOpsLimit, const time::nanoseconds& timeLimit, const time::nanoseconds& tick)
{
  BOOST_ASSERT(!m_isRunning);

  if (nOpsLimit <= 0) {
    return EXCEED_OPS;
  }

  m_isRunning = true;

  m_reason = NO_WORK;
  m_nOpsRemaining = nOpsLimit;
  if (timeLimit >= time::nanoseconds::zero()) {
    m_timeout = scheduler::schedule(timeLimit, bind(&LimitedIo::afterTimeout, this));
  }

  try {
    if (m_uttf == nullptr) {
      getGlobalIoService().run();
    }
    else {
      // timeLimit is enforced by afterTimeout
      m_uttf->advanceClocks(tick, time::nanoseconds::max());
    }
  }
  catch (const StopException&) {
  }
  catch (const std::exception& ex) {
    NFD_LOG_ERROR("g_io.run() exception: " << getExtendedErrorMessage(ex));
    m_reason = EXCEPTION;
    m_lastException = std::current_exception();
  }

  getGlobalIoService().reset();
  scheduler::cancel(m_timeout);
  m_isRunning = false;

  return m_reason;
}

void
LimitedIo::afterOp()
{
  if (!m_isRunning) {
    // Do not proceed further if .afterOp() is invoked out of .run(),
    return;
  }

  --m_nOpsRemaining;

  if (m_nOpsRemaining <= 0) {
    m_reason = EXCEED_OPS;
    getGlobalIoService().stop();
    if (m_uttf != nullptr) {
      BOOST_THROW_EXCEPTION(StopException());
    }
  }
}

void
LimitedIo::afterTimeout()
{
  m_reason = EXCEED_TIME;
  getGlobalIoService().stop();
  if (m_uttf != nullptr) {
    BOOST_THROW_EXCEPTION(StopException());
  }
}

} // namespace tests
} // namespace nfd
