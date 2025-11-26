/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_DEAD_NONCE_LIST_HPP
#define NFD_DAEMON_TABLE_DEAD_NONCE_LIST_HPP

#include "core/common.hpp"

#include <ndn-cxx/util/scheduler.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * DeadNonceList.hpp 解説
 * ------------------------------------------------------------
 * ■役割概要
 *   - NDN におけるループ検知の補助テーブル
 *   - PIT から削除された Interest の Name + Nonce を保持
 *   - 「もう使った Nonce を再度受信したら = ループして戻ってきた」
 *
 * ■特徴
 *   - メモリ削減のため Name + Nonce を 64bit ハッシュ値で格納
 *   - タイムスタンプは保持せず、代わりに
 *        * MARK と呼ばれる特殊な値を定期挿入
 *        * MARK の数で実質的な寿命を推定
 *   - 容量は動的調整（多すぎ → 間引き、少なすぎ → 拡張）
 *
 * ■ループ検知に使う理由
 *   1. PIT から削除された Interest が再び到達
 *   2. DeadNonceList に記録がある
 *   → これはループの可能性あり！ その Interest を破棄できる
 *
 * ■動作概要（ざっくり）
 *   add() で登録
 *       PIT Entry消滅時 → DeadNonceListへ
 *   has() でチェック
 *       受信 Interest が DeadNonceList にあればループ検知成功
 *   mark() & adjustCapacity() で容量制御
 *
 * ■保存期間の考え方
 *   lifetime パラメータ（デフォルト 6秒）
 *   * 通常は数秒でループは収束するため、この間覚えていればOK
 *   * 遅延が大きすぎると検知できない可能性
 *
 * ■内部構造
 *   - boost::multi_index_container による
 *       * Queue (FIFO) … 古い順に削除
 *       * HashTable … 高速検索
 *
 * ------------------------------------------------------------
 * NDN の堅牢性向上に重要な仕組みだが、
 * 可能性ループ検知なので false positiveは仕様上許容している。
 * ------------------------------------------------------------
 */

/*
 * （以下、元コード）
 */

namespace nfd {

/**
 * \brief Represents the Dead Nonce List.
 *
 * The Dead Nonce List is a global table that supplements the PIT for loop detection.
 * When a Nonce is erased (dead) from a PIT entry, the Nonce and the Interest Name are added to
 * the Dead Nonce List and kept for a duration in which most loops are expected to have occured.
 *
 * To reduce memory usage, the Interest Name and Nonce are stored as a 64-bit hash.
 * The probability of false positives (a non-looping Interest considered as looping) is small
 * and a collision is recoverable when the consumer retransmits with a different Nonce.
 *
 * To reduce memory usage, entries do not have associated timestamps. Instead, the lifetime
 * of the entries is controlled by dynamically adjusting the capacity of the container.
 * At fixed intervals, a MARK (an entry with a special value) is inserted into the container.
 * The number of MARKs stored in the container reflects the lifetime of the entries,
 * because MARKs are inserted at fixed intervals.
 */
class DeadNonceList : noncopyable
{
public:
  /**
   * \brief Constructs the Dead Nonce List
   * \param lifetime expected lifetime of each nonce, must be no less than #MIN_LIFETIME.
   *        This should be set to a duration over which most loops would have occured.
   *        A loop cannot be detected if the total delay of the cycle is greater than lifetime.
   * \throw std::invalid_argument if lifetime is less than #MIN_LIFETIME
   */
  explicit
  DeadNonceList(time::nanoseconds lifetime = DEFAULT_LIFETIME);

  /**
   * \brief Determines if name+nonce is in the list
   * \return true if name+nonce exists, false otherwise
   */
  bool
  has(const Name& name, Interest::Nonce nonce) const;

  /**
   * \brief Adds name+nonce to the list
   */
  void
  add(const Name& name, Interest::Nonce nonce);

  /**
   * \brief Returns the number of stored nonces
   * \note The return value does not contain non-Nonce entries in the index, if any.
   */
  size_t
  size() const;

  /**
   * \brief Returns the expected nonce lifetime
   */
  time::nanoseconds
  getLifetime() const
  {
    return m_lifetime;
  }

private:
  using Entry = uint64_t;

  static Entry
  makeEntry(const Name& name, Interest::Nonce nonce);

  /** \brief Return the number of MARKs in the index
   */
  size_t
  countMarks() const;

  /** \brief Add a MARK, then record number of MARKs in m_actualMarkCounts
   */
  void
  mark();

  /** \brief Adjust capacity according to m_actualMarkCounts
   *
   *  If all counts are above EXPECTED_MARK_COUNT, reduce capacity to m_capacity * CAPACITY_DOWN.
   *  If all counts are below EXPECTED_MARK_COUNT, increase capacity to m_capacity * CAPACITY_UP.
   */
  void
  adjustCapacity();

  /** \brief Evict some entries if index is over capacity
   */
  void
  evictEntries();

public:
  /// Default entry lifetime
  static constexpr time::nanoseconds DEFAULT_LIFETIME = 6_s;
  /// Minimum entry lifetime
  static constexpr time::nanoseconds MIN_LIFETIME = 50_ms;

private:
  const time::nanoseconds m_lifetime;

  struct Queue {};
  struct Hashtable {};
  using Container = boost::multi_index_container<
    Entry,
    boost::multi_index::indexed_by<
      boost::multi_index::sequenced<boost::multi_index::tag<Queue>>,
      boost::multi_index::hashed_non_unique<boost::multi_index::tag<Hashtable>,
                                            boost::multi_index::identity<Entry>>
    >
  >;

  Container m_index;
  Container::index<Queue>::type& m_queue = m_index.get<Queue>();
  Container::index<Hashtable>::type& m_ht = m_index.get<Hashtable>();

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:

  // ---- current capacity and hard limits

  /** \brief Current capacity of index
   *
   *  The index size is maintained to be near this capacity.
   *
   *  The capacity is adjusted so that every Entry is expected to be kept for m_lifetime.
   *  This is achieved by mark() and adjustCapacity().
   */
  size_t m_capacity;

  static constexpr size_t INITIAL_CAPACITY = 1 << 14;

  /** \brief Minimum capacity
   *
   *  This is to ensure correct algorithm operations.
   */
  static constexpr size_t MIN_CAPACITY = 1 << 10;

  /** \brief Maximum capacity
   *
   *  This is to limit memory usage.
   */
  static constexpr size_t MAX_CAPACITY = 1 << 24;

  // ---- actual entry lifetime estimation

  /** \brief The MARK for capacity
   *
   *  The MARK doesn't have a distinct type.
   *  Entry is a hash. The hash function should be non-invertible, so that
   *  it's infeasible to craft a "normal" Entry that collides with the MARK.
   */
  static constexpr Entry MARK = 0;

  /// Expected number of MARKs in the index
  static constexpr size_t EXPECTED_MARK_COUNT = 5;

  /** \brief Number of MARKs in the index after each MARK insertion
   *
   *  adjustCapacity() uses this to determine whether and how to adjust capcity,
   *  and then clears this list.
   */
  std::multiset<size_t> m_actualMarkCounts;

  const time::nanoseconds m_markInterval;
  ndn::scheduler::ScopedEventId m_markEvent;

  // ---- capacity adjustments

  static constexpr double CAPACITY_UP = 1.2;
  static constexpr double CAPACITY_DOWN = 0.9;
  const time::nanoseconds m_adjustCapacityInterval;
  ndn::scheduler::ScopedEventId m_adjustCapacityEvent;

  /// Maximum number of entries to evict at each operation if the index is over capacity
  static constexpr size_t EVICT_LIMIT = 64;
};

} // namespace nfd

#endif // NFD_DAEMON_TABLE_DEAD_NONCE_LIST_HPP
