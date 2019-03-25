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

#ifndef NFD_TESTS_DAEMON_RIB_IO_FIXTURE_HPP
#define NFD_TESTS_DAEMON_RIB_IO_FIXTURE_HPP

#include "tests/daemon/global-io-fixture.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>

namespace nfd {
namespace tests {

/** \brief A base test fixture that provides both main and RIB io_service.
 */
class RibIoFixture : public GlobalIoFixture
{
protected:
  RibIoFixture();

  ~RibIoFixture();

protected:
  /** \brief Poll main and RIB thread io_service to process all pending I/O events.
   *
   * This call will execute all pending I/O events, including events that are posted
   * inside the processing event, i.e., main and RIB thread io_service will be polled
   * repeatedly until all pending events are processed.
   *
   * \warning Must be called from the main thread
   */
  void
  poll();

protected:
  /** \brief pointer to global main io_service
   */
  boost::asio::io_service* g_mainIo = nullptr;

  /** \brief pointer to global RIB io_service
   */
  boost::asio::io_service* g_ribIo = nullptr;

  /** \brief global RIB thread
   */
  std::thread g_ribThread;

private:
  bool m_shouldStopRibIo = false;
  bool m_shouldPollRibIo = false;
  std::mutex m_ribPollMutex;
  std::condition_variable m_ribPollStartCv;
  std::condition_variable m_ribPollEndCv;
};

/** \brief RibIoFixture that also overrides steady clock and system clock.
 */
class RibIoTimeFixture : public RibIoFixture, public ClockFixture
{
protected:
  RibIoTimeFixture();

private:
  void
  pollAfterClockTick() override;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_RIB_IO_FIXTURE_HPP
