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

#include "tests/daemon/rib-io-fixture.hpp"
#include "tests/test-common.hpp"
#include "common/global.hpp"

#include <boost/exception/diagnostic_information.hpp>

namespace nfd {
namespace tests {

RibIoFixture::RibIoFixture()
{
  std::mutex m;
  std::condition_variable cv;

  g_mainIo = &getGlobalIoService();
  setMainIoService(g_mainIo);

  g_ribThread = std::thread([&] {
    {
      std::lock_guard<std::mutex> lock(m);
      g_ribIo = &getGlobalIoService();
      setRibIoService(g_ribIo);
      BOOST_ASSERT(&g_io != g_ribIo);
      BOOST_ASSERT(g_ribIo == &getRibIoService());
    }
    cv.notify_all();

    try {
      while (true) {
        {
          std::unique_lock<std::mutex> lock(m_ribPollMutex);
          m_ribPollStartCv.wait(lock, [this] { return m_shouldStopRibIo || m_shouldPollRibIo; });
          if (m_shouldStopRibIo) {
            break;
          }
          BOOST_ASSERT(m_shouldPollRibIo);
        }

        if (g_ribIo->stopped()) {
          g_ribIo->reset();
        }
        while (g_ribIo->poll() > 0)
          ;

        {
          std::lock_guard<std::mutex> lock(m_ribPollMutex);
          m_shouldPollRibIo = false;
        }
        m_ribPollEndCv.notify_all();
      }
    }
    catch (...) {
      BOOST_WARN_MESSAGE(false, boost::current_exception_diagnostic_information());
      NDN_THROW_NESTED(std::runtime_error("Fatal exception in RIB thread"));
    }
  });

  {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [this] { return g_ribIo != nullptr; });
  }
}

RibIoFixture::~RibIoFixture()
{
  {
    std::lock_guard<std::mutex> lock(m_ribPollMutex);
    m_shouldStopRibIo = true;
  }
  m_ribPollStartCv.notify_all();
  g_ribThread.join();
}

void
RibIoFixture::poll()
{
  BOOST_ASSERT(&getGlobalIoService() == &g_io);

  size_t nHandlersRun = 0;
  do {
    {
      std::lock_guard<std::mutex> lock(m_ribPollMutex);
      m_shouldPollRibIo = true;
    }
    m_ribPollStartCv.notify_all();

    if (g_io.stopped()) {
      g_io.reset();
    }

    nHandlersRun = g_io.poll();

    {
      std::unique_lock<std::mutex> lock(m_ribPollMutex);
      m_ribPollEndCv.wait(lock, [this] { return !m_shouldPollRibIo; });
    }
  } while (nHandlersRun > 0);
}

RibIoTimeFixture::RibIoTimeFixture()
  : ClockFixture(g_io)
{
}

void
RibIoTimeFixture::pollAfterClockTick()
{
  poll();
}

} // namespace tests
} // namespace nfd
