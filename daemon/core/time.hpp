/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_TIME_H
#define NFD_CORE_TIME_H

#include "common.hpp"

namespace ndn {
namespace time {

/** \class Duration
 *  \brief represents a time interval
 *  Time unit is nanosecond.
 */
typedef int64_t Duration;

/** \class Point
 *  \brief represents a point in time
 *  This uses monotonic clock.
 */
typedef Duration Point;

/// \return{ the current time in monotonic clock }
Point
now();

} // namespace time
} // namespace ndn

#endif // NFD_CORE_TIME_H
