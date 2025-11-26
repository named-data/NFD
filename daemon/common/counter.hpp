/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * NFD Counter Classes
 * ---------------------------------------------------------------------------
 * 本ファイルでは、Named Data Networking Forwarding Daemon (NFD) における
 * 各種カウンタを提供します。これらのカウンタはパケット数、バイト数、
 * およびテーブルサイズなどの計測に使用されます。
 *
 * 特徴:
 *  1. SimpleCounter
 *     - 単純な整数値をラップした基本カウンタ
 *     - コピー禁止 (noncopyable) により、誤ってコピーした場合のカウント
 *       のズレを防止
 *     - 暗黙的に整数として取得可能
 *
 *  2. PacketCounter
 *     - パケット数カウンタ
 *     - ++ 演算子で簡単にインクリメント可能
 *
 *  3. ByteCounter
 *     - バイト数カウンタ
 *     - += 演算子で任意のバイト数を加算可能
 *
 *  4. SizeCounter<T>
 *     - テーブルサイズを監視するカウンタ
 *     - T は size() メンバ関数を持つ型であることが前提
 *     - observe() で監視対象のテーブルを設定可能
 *     - カウント値は常に最新のテーブルサイズを反映
 *
 * 利用例:
 *  PacketCounter pktCount;
 *  ++pktCount;  // パケット1個カウント
 *
 *  ByteCounter byteCount;
 *  byteCount += 1500;  // 1500バイト加算
 *
 *  SizeCounter<std::unordered_map<int,int>> tableSize(&table);
 *  auto size = tableSize; // 現在のテーブルサイズを取得
 *
 * 注意:
 *  - PacketCounter / ByteCounter はオーバーフローの可能性あり
 *  - SizeCounter は observe() で対象テーブルを変更可能
 * ---------------------------------------------------------------------------
 */

namespace nfd {

/**
 * \brief Represents a counter that encloses an integer value.
 *
 * SimpleCounter is noncopyable, because increment should be called on the counter,
 * not a copy of it. It's implicitly convertible to an integral type to be observed.
 */
class SimpleCounter : noncopyable
{
public:
  using rep = uint64_t;

  /**
   * \brief Return the counter's value.
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

/**
 * \brief Represents a counter of number of packets.
 *
 * \warning The counter value may wrap after exceeding the range of the underlying integer type.
 */
class PacketCounter : public SimpleCounter
{
public:
  /**
   * \brief Increment the counter by one.
   */
  PacketCounter&
  operator++() noexcept
  {
    ++m_value;
    return *this;
  }
};

/**
 * \brief Represents a counter of number of bytes.
 *
 * \warning The counter value may wrap after exceeding the range of the underlying integer type.
 */
class ByteCounter : public SimpleCounter
{
public:
  /**
   * \brief Increase the counter.
   */
  ByteCounter&
  operator+=(rep n) noexcept
  {
    m_value += n;
    return *this;
  }
};

/**
 * \brief Provides a counter that observes the size of a table.
 * \tparam T a type that provides a `size()` const member function
 *
 * If the table is not specified in the constructor, it can be added later by calling observe().
 */
template<typename T>
class SizeCounter : noncopyable
{
public:
  using rep = size_t;

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

  /**
   * \brief Return the counter's value, i.e., the current size of the table being observed.
   */
  operator rep() const
  {
    BOOST_ASSERT(m_table != nullptr);
    return m_table->size();
  }

private:
  const T* m_table = nullptr;
};

} // namespace nfd

#endif // NFD_DAEMON_COMMON_COUNTER_HPP
