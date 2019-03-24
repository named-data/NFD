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

#ifndef NFD_TESTS_CLOCK_FIXTURE_HPP
#define NFD_TESTS_CLOCK_FIXTURE_HPP

#include "core/common.hpp"

#include <ndn-cxx/util/time-unit-test-clock.hpp>

namespace nfd {
namespace tests {

/** \brief A test fixture that overrides steady clock and system clock.
 */
class ClockFixture
{
public:
  virtual
  ~ClockFixture();

  /** \brief Advance steady and system clocks.
   *
   *  Clocks are advanced in increments of \p tick for \p nTicks ticks.
   *  After each tick, global io_service is polled to process pending I/O events.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancing would stop in case of an exception.
   */
  void
  advanceClocks(time::nanoseconds tick, size_t nTicks = 1)
  {
    advanceClocks(tick, tick * nTicks);
  }

  /** \brief Advance steady and system clocks.
   *
   *  Clocks are advanced in increments of \p tick for \p total time.
   *  The last increment might be shorter than \p tick.
   *  After each tick, global io_service is polled to process pending I/O events.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancing would stop in case of an exception.
   */
  void
  advanceClocks(time::nanoseconds tick, time::nanoseconds total);

protected:
  explicit
  ClockFixture(boost::asio::io_service& io);

private:
  /** \brief Called by advanceClocks() after each clock advancement (tick).
   */
  virtual void
  pollAfterClockTick();

protected:
  shared_ptr<time::UnitTestSteadyClock> m_steadyClock;
  shared_ptr<time::UnitTestSystemClock> m_systemClock;

private:
  boost::asio::io_service& m_io;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_CLOCK_FIXTURE_HPP
