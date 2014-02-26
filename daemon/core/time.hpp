/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_TIME_H
#define NFD_CORE_TIME_H

#include "common.hpp"
#include <ndn-cpp-dev/util/time.hpp>

namespace nfd {
namespace time {

using ndn::time::Duration;
using ndn::time::Point;
using ndn::time::monotonic_clock;
using ndn::time::now;
using ndn::time::seconds;
using ndn::time::milliseconds;
using ndn::time::microseconds;
using ndn::time::nanoseconds;

} // namespace time
} // namespace nfd

#endif // NFD_CORE_TIME_H
