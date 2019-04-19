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

#include "common/global.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/rib-io-fixture.hpp"

#include <thread>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestGlobal, GlobalIoFixture)

BOOST_AUTO_TEST_CASE(ThreadLocalIoService)
{
  boost::asio::io_service* s1 = &getGlobalIoService();
  boost::asio::io_service* s2 = nullptr;

  std::thread t([&s2] { s2 = &getGlobalIoService(); });
  t.join();

  BOOST_CHECK(s1 != nullptr);
  BOOST_CHECK(s2 != nullptr);
  BOOST_CHECK(s1 != s2);
}

BOOST_AUTO_TEST_CASE(ThreadLocalScheduler)
{
  scheduler::Scheduler* s1 = &getScheduler();
  scheduler::Scheduler* s2 = nullptr;

  std::thread t([&s2] { s2 = &getScheduler(); });
  t.join();

  BOOST_CHECK(s1 != nullptr);
  BOOST_CHECK(s2 != nullptr);
  BOOST_CHECK(s1 != s2);
}

BOOST_FIXTURE_TEST_CASE(MainRibIoService, RibIoFixture)
{
  boost::asio::io_service* mainIo = &g_io;
  boost::asio::io_service* ribIo = g_ribIo;

  BOOST_CHECK(mainIo != ribIo);
  BOOST_CHECK(&getGlobalIoService() == mainIo);
  BOOST_CHECK(&getMainIoService() == mainIo);
  BOOST_CHECK(&getRibIoService() == ribIo);
  auto mainThreadId = std::this_thread::get_id();

  runOnRibIoService([&] {
    BOOST_CHECK(mainThreadId != std::this_thread::get_id());
    BOOST_CHECK(&getGlobalIoService() == ribIo);
    BOOST_CHECK(&getMainIoService() == mainIo);
    BOOST_CHECK(&getRibIoService() == ribIo);
  });

  runOnRibIoService([&] {
    runOnMainIoService([&] {
      BOOST_CHECK(mainThreadId == std::this_thread::get_id());
      BOOST_CHECK(&getGlobalIoService() == mainIo);
      BOOST_CHECK(&getMainIoService() == mainIo);
      BOOST_CHECK(&getRibIoService() == ribIo);
    });
  });
}

BOOST_FIXTURE_TEST_CASE(PollInAllThreads, RibIoFixture)
{
  bool hasRibRun = false;
  runOnRibIoService([&] { hasRibRun = true; });
  std::this_thread::sleep_for(std::chrono::seconds(1));
  BOOST_CHECK_EQUAL(hasRibRun, false);

  poll();
  BOOST_CHECK_EQUAL(hasRibRun, true);

  hasRibRun = false;
  bool hasMainRun = false;
  runOnMainIoService([&] {
    hasMainRun = true;
    runOnRibIoService([&] { hasRibRun = true; });
  });
  BOOST_CHECK_EQUAL(hasMainRun, false);
  BOOST_CHECK_EQUAL(hasRibRun, false);

  poll();
  BOOST_CHECK_EQUAL(hasMainRun, true);
  BOOST_CHECK_EQUAL(hasRibRun, true);
}

BOOST_FIXTURE_TEST_CASE(AdvanceClocks, RibIoTimeFixture)
{
  bool hasRibRun = false;
  runOnRibIoService([&] { hasRibRun = true; });
  std::this_thread::sleep_for(std::chrono::seconds(1));
  BOOST_CHECK_EQUAL(hasRibRun, false);

  advanceClocks(1_ns, 1);
  BOOST_CHECK_EQUAL(hasRibRun, true);

  hasRibRun = false;
  bool hasMainRun = false;
  getScheduler().schedule(250_ms, [&] {
    hasMainRun = true;
    runOnRibIoService([&] { hasRibRun = true; });
  });
  BOOST_CHECK_EQUAL(hasMainRun, false);
  BOOST_CHECK_EQUAL(hasRibRun, false);

  advanceClocks(260_ms, 2);
  BOOST_CHECK_EQUAL(hasMainRun, true);
  BOOST_CHECK_EQUAL(hasRibRun, true);
}

BOOST_AUTO_TEST_SUITE_END() // TestGlobal

} // namespace tests
} // namespace nfd
