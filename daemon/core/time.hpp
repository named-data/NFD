/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_TIME_H
#define NFD_CORE_TIME_H

#include "common.hpp"

namespace nfd {
namespace time {

class monotonic_clock;

/** \class Duration
 *  \brief represents a time interval
 *  Time unit is nanosecond.
 */
class Duration
{
public:
  Duration()
    : m_value(0)
  {
  }

  explicit
  Duration(int64_t value)
    : m_value(value)
  {
  }
  
  operator int64_t&()
  {
    return m_value;
  }

  operator const int64_t&() const
  {
    return m_value;
  }

  Duration
  operator+(const Duration& other) const
  {
    return Duration(this->m_value + other.m_value);
  }
  
  Duration
  operator-(const Duration& other) const
  {
    return Duration(this->m_value - other.m_value);
  }

private:
  int64_t m_value;
};

/** \class Point
 *  \brief represents a point in time
 *  This uses monotonic clock.
 */
class Point
{
public:
  Point()
    : m_value(0)
  {
  }

  explicit
  Point(int64_t value)
    : m_value(value)
  {
  }
  
  operator int64_t&()
  {
    return m_value;
  }

  operator const int64_t&() const
  {
    return m_value;
  }

  Point
  operator+(const Duration& other) const
  {
    return Point(this->m_value + static_cast<int64_t>(other));
  }
  
  Duration
  operator-(const Point& other) const
  {
    return Duration(this->m_value - other.m_value);
  }

  Point
  operator-(const Duration& other) const
  {
    return Point(this->m_value  - static_cast<int64_t>(other));
  }
  
private:
  int64_t m_value;
};

/**
 * \brief Get current time
 * \return{ the current time in monotonic clock }
 */
Point
now();

/**
 * \brief Get time::Duration for the specified number of seconds
 */
template<class T>
inline Duration
seconds(T value)
{
  return Duration(value * static_cast<int64_t>(1000000000));
}

/**
 * \brief Get time::Duration for the specified number of milliseconds
 */
template<class T>
inline Duration
milliseconds(T value)
{
  return Duration(value * static_cast<int64_t>(1000000));
}

/**
 * \brief Get time::Duration for the specified number of microseconds
 */
template<class T>
inline Duration
microseconds(T value)
{
  return Duration(value * static_cast<int64_t>(1000));
}

/**
 * \brief Get time::Duration for the specified number of nanoseconds
 */
inline Duration
nanoseconds(int64_t value)
{
  return Duration(value);
}


} // namespace time
} // namespace nfd

#endif // NFD_CORE_TIME_H
