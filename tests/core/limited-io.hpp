/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_CORE_LIMITED_IO_HPP
#define NFD_TEST_CORE_LIMITED_IO_HPP

#include "core/global-io.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace tests {

/** \brief provides IO operations limit and/or time limit for unit testing
 */
class LimitedIo
{
public:
  LimitedIo();

  /// indicates why .run returns
  enum StopReason
  {
    /// g_io.run() runs normally because there's no work to do
    NO_WORK,
    /// .afterOp() has been invoked nOpsLimit times
    EXCEED_OPS,
    /// nTimeLimit has elapsed
    EXCEED_TIME,
    /// an exception is thrown
    EXCEPTION
  };

  /** \brief g_io.run() with operation count and/or time limit
   *
   *  \param nOpsLimit operation count limit, pass UNLIMITED_OPS for no limit
   *  \param nTimeLimit time limit, pass UNLIMITED_TIME for no limit
   */
  StopReason
  run(int nOpsLimit, const time::nanoseconds& nTimeLimit);

  /// count an operation
  void
  afterOp();

  const std::exception&
  getLastException() const;

private:
  void
  afterTimeout();

public:
  static const int UNLIMITED_OPS;
  static const time::nanoseconds UNLIMITED_TIME;

private:
  bool m_isRunning;
  int m_nOpsRemaining;
  EventId m_timeout;
  StopReason m_reason;
  std::exception m_lastException;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TEST_CORE_LIMITED_IO_HPP
