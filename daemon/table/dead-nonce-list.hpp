/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#include "common.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include "core/scheduler.hpp"

namespace nfd {

/** \brief represents the Dead Nonce list
 *
 *  The Dead Nonce List is a global table that supplements PIT for loop detection.
 *  When a Nonce is erased (dead) from PIT entry, the Nonce and the Interest Name is added to
 *  Dead Nonce List, and kept for a duration in which most loops are expected to have occured.
 *
 *  To reduce memory usage, the Interest Name and Nonce are stored as a 64-bit hash.
 *  There could be false positives (non-looping Interest could be considered looping),
 *  but the probability is small, and the error is recoverable when consumer retransmits
 *  with a different Nonce.
 *
 *  To reduce memory usage, entries do not have associated timestamps. Instead,
 *  lifetime of entries is controlled by dynamically adjusting the capacity of the container.
 *  At fixed intervals, the MARK, an entry with a special value, is inserted into the container.
 *  The number of MARKs stored in the container reflects the lifetime of entries,
 *  because MARKs are inserted at fixed intervals.
 */
class DeadNonceList : noncopyable
{
public:
  /** \brief constructs the Dead Nonce List
   *  \param lifetime duration of the expected lifetime of each nonce,
   *         must be no less than MIN_LIFETIME.
   *         This should be set to the duration in which most loops would have occured.
   *         A loop cannot be detected if delay of the cycle is greater than lifetime.
   *  \throw std::invalid_argument if lifetime is less than MIN_LIFETIME
   */
  explicit
  DeadNonceList(const time::nanoseconds& lifetime = DEFAULT_LIFETIME);

  ~DeadNonceList();

  /** \brief determines if name+nonce exists
   *  \return true if name+nonce exists
   */
  bool
  has(const Name& name, uint32_t nonce) const;

  /** \brief records name+nonce
   */
  void
  add(const Name& name, uint32_t nonce);

  /** \return number of stored Nonces
   *  \note The return value does not contain non-Nonce entries in the index, if any.
   */
  size_t
  size() const;

  /** \return expected lifetime
   */
  const time::nanoseconds&
  getLifetime() const;

private: // Entry and Index
  typedef uint64_t Entry;

  static Entry
  makeEntry(const Name& name, uint32_t nonce);

  typedef boost::multi_index_container<
    Entry,
    boost::multi_index::indexed_by<
      boost::multi_index::sequenced<>,
      boost::multi_index::hashed_non_unique<
        boost::multi_index::identity<Entry>
      >
    >
  > Index;

  typedef Index::nth_index<0>::type Queue;
  typedef Index::nth_index<1>::type Hashtable;

private: // actual lifetime estimation and capacity control
  /** \return number of MARKs in the index
   */
  size_t
  countMarks() const;

  /** \brief add a MARK, then record number of MARKs in m_actualMarkCounts
   */
  void
  mark();

  /** \brief adjust capacity according to m_actualMarkCounts
   *
   *  If all counts are above EXPECTED_MARK_COUNT, reduce capacity to m_capacity * CAPACITY_DOWN.
   *  If all counts are below EXPECTED_MARK_COUNT, increase capacity to m_capacity * CAPACITY_UP.
   */
  void
  adjustCapacity();

  /** \brief evict some entries if index is over capacity
   */
  void
  evictEntries();

public:
  /// default entry lifetime
  static const time::nanoseconds DEFAULT_LIFETIME;

  /// minimum entry lifetime
  static const time::nanoseconds MIN_LIFETIME;

private:
  time::nanoseconds m_lifetime;
  Index m_index;
  Queue& m_queue;
  Hashtable& m_ht;

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // actual lifetime estimation and capacity control

  // ---- current capacity and hard limits

  /** \brief current capacity of index
   *
   *  The index size is maintained to be near this capacity.
   *
   *  The capacity is adjusted so that every Entry is expected to be kept for m_lifetime.
   *  This is achieved by mark() and adjustCapacity().
   */
  size_t m_capacity;

  static const size_t INITIAL_CAPACITY;

  /** \brief minimum capacity
   *
   *  This is to ensure correct algorithm operations.
   */
  static const size_t MIN_CAPACITY;

  /** \brief maximum capacity
   *
   *  This is to limit memory usage.
   */
  static const size_t MAX_CAPACITY;

  // ---- actual entry lifetime estimation

  /** \brief the MARK for capacity
   *
   *  The MARK doesn't have a distinct type.
   *  Entry is a hash. The hash function should have non-invertible property,
   *  so it's unlikely for a usual Entry to have collision with the MARK.
   */
  static const Entry MARK;

  /** \brief expected number of MARKs in the index
   */
  static const size_t EXPECTED_MARK_COUNT;

  /** \brief number of MARKs in the index after each MARK insertion
   *
   *  adjustCapacity uses this to determine whether and how to adjust capcity,
   *  and then clears this list.
   */
  std::multiset<size_t> m_actualMarkCounts;

  time::nanoseconds m_markInterval;

  scheduler::EventId m_markEvent;

  // ---- capacity adjustments

  static const double CAPACITY_UP;

  static const double CAPACITY_DOWN;

  time::nanoseconds m_adjustCapacityInterval;

  scheduler::EventId m_adjustCapacityEvent;

  /** \brief maximum number of entries to evict at each operation if index is over capacity
   */
  static const size_t EVICT_LIMIT;
};

inline const time::nanoseconds&
DeadNonceList::getLifetime() const
{
  return m_lifetime;
}

} // namespace nfd

#endif // NFD_DAEMON_TABLE_DEAD_NONCE_LIST_HPP
