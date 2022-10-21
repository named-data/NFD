/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_DAEMON_COMMON_COUNTER_HPP
#define NFD_DAEMON_COMMON_COUNTER_HPP

#include "core/common.hpp"

namespace nfd {

/**
 * \brief Represents a counter that encloses an integer value.
 *
 * SimpleCounter is noncopyable, because increment should be called on the counter,
 * not a copy of it; it's implicitly convertible to an integral type to be observed.
 */
class SimpleCounter : noncopyable
{
public:
  typedef uint64_t rep;

  /**
   * \brief Observe the counter's value.
   */
  operator rep() const noexcept
  {
    return m_value;
  }

  /**
   * \brief Replace the counter's value.
   */
  void
  set(rep value) noexcept
  {
    m_value = value;
  }

protected:
  rep m_value = 0;
};

/** \brief Represents a counter of number of packets.
 *
 *  \warning The counter value may wrap after exceeding the range of underlying integer type.
 */
class PacketCounter : public SimpleCounter
{
public:
  /** \brief Increment the counter by one.
   */
  PacketCounter&
  operator++() noexcept
  {
    ++m_value;
    return *this;
  }
  // postfix ++ operator is not provided because it's not needed
};

/** \brief Represents a counter of number of bytes.
 *
 *  \warning The counter value may wrap after exceeding the range of underlying integer type.
 */
class ByteCounter : public SimpleCounter
{
public:
  /** \brief Increase the counter.
   */
  ByteCounter&
  operator+=(rep n) noexcept
  {
    m_value += n;
    return *this;
  }
};

/** \brief Provides a counter that observes the size of a table.
 *  \tparam T a type that provides a size() const member function
 *
 *  if table not specified in constructor, it can be added later by invoking observe()
 */
template<typename T>
class SizeCounter : noncopyable
{
public:
  typedef size_t Rep;

  explicit constexpr
  SizeCounter(const T* table = nullptr) noexcept
    : m_table(table)
  {
  }

  void
  observe(const T* table) noexcept
  {
    m_table = table;
  }

  /** \brief Observe the counter.
   */
  operator Rep() const
  {
    BOOST_ASSERT(m_table != nullptr);
    return m_table->size();
  }

private:
  const T* m_table;
};

} // namespace nfd

#endif // NFD_DAEMON_COMMON_COUNTER_HPP
