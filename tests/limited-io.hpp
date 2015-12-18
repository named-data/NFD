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

#ifndef NFD_TESTS_LIMITED_IO_HPP
#define NFD_TESTS_LIMITED_IO_HPP

#include "test-common.hpp"
#include "core/scheduler.hpp"

#include <exception>

namespace nfd {
namespace tests {

/** \brief provides IO operations limit and/or time limit for unit testing
 */
class LimitedIo : noncopyable
{
public:
  /** \brief construct with UnitTestTimeFixture
   */
  explicit
  LimitedIo(UnitTestTimeFixture* uttf = nullptr);

  /// indicates why .run returns
  enum StopReason {
    /// g_io.run() returns normally because there's no work to do
    NO_WORK,
    /// .afterOp() has been invoked nOpsLimit times
    EXCEED_OPS,
    /// nTimeLimit has elapsed
    EXCEED_TIME,
    /// an exception is thrown
    EXCEPTION
  };

  /** \brief g_io.run() with operation count and/or time limit
   *  \param nOpsLimit operation count limit, pass UNLIMITED_OPS for no limit
   *  \param timeLimit time limit, pass UNLIMITED_TIME for no limit
   *  \param tick if this LimitedIo is constructed with UnitTestTimeFixture,
   *              this is passed to .advanceClocks(), otherwise ignored
   */
  StopReason
  run(int nOpsLimit, const time::nanoseconds& timeLimit,
      const time::nanoseconds& tick = time::milliseconds(1));

  /// count an operation
  void
  afterOp();

  /** \brief defer for specified duration
   *
   *  equivalent to .run(UNLIMITED_OPS, d)
   */
  void
  defer(const time::nanoseconds& d)
  {
    this->run(UNLIMITED_OPS, d);
  }

  std::exception_ptr
  getLastException() const
  {
    return m_lastException;
  }

private:
  /** \brief an exception to stop IO operation
   */
  class StopException : public std::exception
  {
  };

  void
  afterTimeout();

public:
  static const int UNLIMITED_OPS;
  static const time::nanoseconds UNLIMITED_TIME;

private:
  UnitTestTimeFixture* m_uttf;
  StopReason m_reason;
  int m_nOpsRemaining;
  scheduler::EventId m_timeout;
  std::exception_ptr m_lastException;
  bool m_isRunning;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_LIMITED_IO_HPP
