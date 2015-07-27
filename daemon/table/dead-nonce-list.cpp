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

#include "dead-nonce-list.hpp"
#include "core/city-hash.hpp"
#include "core/logger.hpp"

NFD_LOG_INIT("DeadNonceList");

namespace nfd {

const time::nanoseconds DeadNonceList::DEFAULT_LIFETIME = time::seconds(6);
const time::nanoseconds DeadNonceList::MIN_LIFETIME = time::milliseconds(1);
const size_t DeadNonceList::INITIAL_CAPACITY = (1 << 7);
const size_t DeadNonceList::MIN_CAPACITY = (1 << 3);
const size_t DeadNonceList::MAX_CAPACITY = (1 << 24);
const DeadNonceList::Entry DeadNonceList::MARK = 0;
const size_t DeadNonceList::EXPECTED_MARK_COUNT = 5;
const double DeadNonceList::CAPACITY_UP = 1.2;
const double DeadNonceList::CAPACITY_DOWN = 0.9;
const size_t DeadNonceList::EVICT_LIMIT = (1 << 6);

DeadNonceList::DeadNonceList(const time::nanoseconds& lifetime)
  : m_lifetime(lifetime)
  , m_queue(m_index.get<0>())
  , m_ht(m_index.get<1>())
  , m_capacity(INITIAL_CAPACITY)
  , m_markInterval(m_lifetime / EXPECTED_MARK_COUNT)
  , m_adjustCapacityInterval(m_lifetime)
{
  if (m_lifetime < MIN_LIFETIME) {
    BOOST_THROW_EXCEPTION(std::invalid_argument("lifetime is less than MIN_LIFETIME"));
  }

  for (size_t i = 0; i < EXPECTED_MARK_COUNT; ++i) {
    m_queue.push_back(MARK);
  }

  m_markEvent = scheduler::schedule(m_markInterval, bind(&DeadNonceList::mark, this));
  m_adjustCapacityEvent = scheduler::schedule(m_adjustCapacityInterval,
                                              bind(&DeadNonceList::adjustCapacity, this));
}

DeadNonceList::~DeadNonceList()
{
  scheduler::cancel(m_markEvent);
  scheduler::cancel(m_adjustCapacityEvent);

  BOOST_ASSERT_MSG(DEFAULT_LIFETIME >= MIN_LIFETIME, "DEFAULT_LIFETIME is too small");
  static_assert(INITIAL_CAPACITY >= MIN_CAPACITY, "INITIAL_CAPACITY is too small");
  static_assert(INITIAL_CAPACITY <= MAX_CAPACITY, "INITIAL_CAPACITY is too large");
  BOOST_ASSERT_MSG(static_cast<size_t>(MIN_CAPACITY * CAPACITY_UP) > MIN_CAPACITY,
                   "CAPACITY_UP must be able to increase from MIN_CAPACITY");
  BOOST_ASSERT_MSG(static_cast<size_t>(MAX_CAPACITY * CAPACITY_DOWN) < MAX_CAPACITY,
                   "CAPACITY_DOWN must be able to decrease from MAX_CAPACITY");
  BOOST_ASSERT_MSG(CAPACITY_UP > 1.0, "CAPACITY_UP must adjust up");
  BOOST_ASSERT_MSG(CAPACITY_DOWN < 1.0, "CAPACITY_DOWN must adjust down");
  static_assert(EVICT_LIMIT >= 1, "EVICT_LIMIT must be at least 1");
}

size_t
DeadNonceList::size() const
{
  return m_queue.size() - this->countMarks();
}

bool
DeadNonceList::has(const Name& name, uint32_t nonce) const
{
  Entry entry = DeadNonceList::makeEntry(name, nonce);
  return m_ht.find(entry) != m_ht.end();
}

void
DeadNonceList::add(const Name& name, uint32_t nonce)
{
  Entry entry = DeadNonceList::makeEntry(name, nonce);
  m_queue.push_back(entry);

  this->evictEntries();
}

DeadNonceList::Entry
DeadNonceList::makeEntry(const Name& name, uint32_t nonce)
{
  Block nameWire = name.wireEncode();
  return CityHash64WithSeed(reinterpret_cast<const char*>(nameWire.wire()), nameWire.size(),
                            static_cast<uint64_t>(nonce));
}

size_t
DeadNonceList::countMarks() const
{
  return m_ht.count(MARK);
}

void
DeadNonceList::mark()
{
  m_queue.push_back(MARK);
  size_t nMarks = this->countMarks();
  m_actualMarkCounts.insert(nMarks);

  NFD_LOG_TRACE("mark nMarks=" << nMarks);

  scheduler::schedule(m_markInterval, bind(&DeadNonceList::mark, this));
}

void
DeadNonceList::adjustCapacity()
{
  std::pair<std::multiset<size_t>::iterator, std::multiset<size_t>::iterator> equalRange =
    m_actualMarkCounts.equal_range(EXPECTED_MARK_COUNT);

  if (equalRange.second == m_actualMarkCounts.begin()) {
    // all counts are above expected count, adjust down
    m_capacity = std::max(MIN_CAPACITY,
                          static_cast<size_t>(m_capacity * CAPACITY_DOWN));
    NFD_LOG_TRACE("adjustCapacity DOWN capacity=" << m_capacity);
  }
  else if (equalRange.first == m_actualMarkCounts.end()) {
    // all counts are below expected count, adjust up
    m_capacity = std::min(MAX_CAPACITY,
                          static_cast<size_t>(m_capacity * CAPACITY_UP));
    NFD_LOG_TRACE("adjustCapacity UP capacity=" << m_capacity);
  }

  m_actualMarkCounts.clear();

  this->evictEntries();

  m_adjustCapacityEvent = scheduler::schedule(m_adjustCapacityInterval,
                                              bind(&DeadNonceList::adjustCapacity, this));
}

void
DeadNonceList::evictEntries()
{
  ssize_t nOverCapacity = m_queue.size() - m_capacity;
  if (nOverCapacity <= 0) // not over capacity
    return;

  for (ssize_t nEvict = std::min<ssize_t>(nOverCapacity, EVICT_LIMIT); nEvict > 0; --nEvict) {
    m_queue.erase(m_queue.begin());
  }
  BOOST_ASSERT(m_queue.size() >= m_capacity);
}

} // namespace nfd
