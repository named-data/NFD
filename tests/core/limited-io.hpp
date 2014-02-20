/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_CORE_LIMITED_IO_HPP
#define NFD_TEST_CORE_LIMITED_IO_HPP

#include "core/scheduler.hpp"

namespace nfd {

class LimitedIo
{
public:
  LimitedIo();
  
  enum StopReason
  {
    NO_WORK,
    EXCEED_OPS,
    EXCEED_TIME
  };
  
  StopReason
  run(int nOpsLimit, time::Duration nTimeLimit);
  
  void
  afterOp();
  
private:
  void
  afterTimeout();

public:
  static const int UNLIMITED_OPS;
  static const time::Duration UNLIMITED_TIME;

private:
  bool m_isRunning;
  int m_nOpsRemaining;
  EventId m_timeout;
  StopReason m_reason;
};

} // namespace nfd

#endif // NFD_TEST_CORE_LIMITED_IO_HPP
