/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "tests/daemon/limited-io.hpp"
#include "tests/test-common.hpp"
#include "common/global.hpp"

#include <boost/exception/diagnostic_information.hpp>

namespace nfd {
namespace tests {

const int LimitedIo::UNLIMITED_OPS = std::numeric_limits<int>::max();
const time::nanoseconds LimitedIo::UNLIMITED_TIME = time::nanoseconds::min();

LimitedIo::LimitedIo(GlobalIoTimeFixture* fixture)
  : m_fixture(fixture)
{
}

LimitedIo::StopReason
LimitedIo::run(int nOpsLimit, time::nanoseconds timeLimit, time::nanoseconds tick)
{
  BOOST_ASSERT(!m_isRunning);

  if (nOpsLimit <= 0) {
    return EXCEED_OPS;
  }

  m_isRunning = true;

  m_reason = NO_WORK;
  m_nOpsRemaining = nOpsLimit;
  if (timeLimit >= 0_ns) {
    m_timeout = getScheduler().schedule(timeLimit, [this] { afterTimeout(); });
  }

  try {
    if (m_fixture == nullptr) {
      getGlobalIoService().run();
    }
    else {
      // timeLimit is enforced by afterTimeout
      m_fixture->advanceClocks(tick, time::nanoseconds::max());
    }
  }
  catch (const StopException&) {
  }
  catch (...) {
    BOOST_WARN_MESSAGE(false, boost::current_exception_diagnostic_information());
    m_reason = EXCEPTION;
    m_lastException = std::current_exception();
  }

  getGlobalIoService().reset();
  m_timeout.cancel();
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
    if (m_fixture != nullptr) {
      NDN_THROW(StopException());
    }
  }
}

void
LimitedIo::afterTimeout()
{
  m_reason = EXCEED_TIME;
  getGlobalIoService().stop();
  if (m_fixture != nullptr) {
    NDN_THROW(StopException());
  }
}

} // namespace tests
} // namespace nfd
