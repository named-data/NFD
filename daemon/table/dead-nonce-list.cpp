/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
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

#include "dead-nonce-list.hpp"
#include "common/city-hash.hpp"
#include "common/global.hpp"
#include "common/logger.hpp"

namespace nfd {

NFD_LOG_INIT(DeadNonceList);

const time::nanoseconds DeadNonceList::DEFAULT_LIFETIME;
const time::nanoseconds DeadNonceList::MIN_LIFETIME;
const size_t DeadNonceList::INITIAL_CAPACITY;
const size_t DeadNonceList::MIN_CAPACITY;
const size_t DeadNonceList::MAX_CAPACITY;
const DeadNonceList::Entry DeadNonceList::MARK;
const size_t DeadNonceList::EXPECTED_MARK_COUNT;
const double DeadNonceList::CAPACITY_UP;
const double DeadNonceList::CAPACITY_DOWN;
const size_t DeadNonceList::EVICT_LIMIT;

DeadNonceList::DeadNonceList(time::nanoseconds lifetime)
  : m_lifetime(lifetime)
  , m_capacity(INITIAL_CAPACITY)
  , m_markInterval(m_lifetime / EXPECTED_MARK_COUNT)
  , m_adjustCapacityInterval(m_lifetime)
{
  if (m_lifetime < MIN_LIFETIME) {
    NDN_THROW(std::invalid_argument("lifetime is less than MIN_LIFETIME"));
  }

  for (size_t i = 0; i < EXPECTED_MARK_COUNT; ++i) {
    m_queue.push_back(MARK);
  }

  m_markEvent = getScheduler().schedule(m_markInterval, [this] { mark(); });
  m_adjustCapacityEvent = getScheduler().schedule(m_adjustCapacityInterval, [this] { adjustCapacity(); });

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
  return m_queue.size() - countMarks();
}

bool
DeadNonceList::has(const Name& name, Interest::Nonce nonce) const
{
  Entry entry = DeadNonceList::makeEntry(name, nonce);
  return m_ht.find(entry) != m_ht.end();
}

void
DeadNonceList::add(const Name& name, Interest::Nonce nonce)
{
  Entry entry = DeadNonceList::makeEntry(name, nonce);
  const auto iter = m_ht.find(entry);
  bool isDuplicate = iter != m_ht.end();

  NFD_LOG_TRACE("adding " << (isDuplicate ? "duplicate " : "") << name << " nonce=" << nonce);

  if (isDuplicate) {
    m_queue.relocate(m_queue.end(), m_index.project<Queue>(iter));
  }
  else {
    m_queue.push_back(entry);
    evictEntries();
  }
}

DeadNonceList::Entry
DeadNonceList::makeEntry(const Name& name, Interest::Nonce nonce)
{
  const auto& nameWire = name.wireEncode();
  uint32_t n;
  std::memcpy(&n, nonce.data(), sizeof(n));
  return CityHash64WithSeed(reinterpret_cast<const char*>(nameWire.wire()), nameWire.size(), n);
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
  size_t nMarks = countMarks();
  m_actualMarkCounts.insert(nMarks);

  NFD_LOG_TRACE("mark nMarks=" << nMarks);

  m_markEvent = getScheduler().schedule(m_markInterval, [this] { mark(); });
}

void
DeadNonceList::adjustCapacity()
{
  auto oldCapacity = m_capacity;
  auto equalRange = m_actualMarkCounts.equal_range(EXPECTED_MARK_COUNT);
  if (equalRange.second == m_actualMarkCounts.begin()) {
    // all counts are above expected count, adjust down
    m_capacity = std::max(MIN_CAPACITY, static_cast<size_t>(m_capacity * CAPACITY_DOWN));
  }
  else if (equalRange.first == m_actualMarkCounts.end()) {
    // all counts are below expected count, adjust up
    m_capacity = std::min(MAX_CAPACITY, static_cast<size_t>(m_capacity * CAPACITY_UP));
  }

  if (m_capacity != oldCapacity) {
    NFD_LOG_TRACE("adjusting capacity " << oldCapacity << " -> " << m_capacity);
  }

  m_actualMarkCounts.clear();
  evictEntries();

  m_adjustCapacityEvent = getScheduler().schedule(m_adjustCapacityInterval, [this] { adjustCapacity(); });
}

void
DeadNonceList::evictEntries()
{
  if (m_queue.size() <= m_capacity) // not over capacity
    return;

  auto nEvict = std::min(m_queue.size() - m_capacity, EVICT_LIMIT);
  for (size_t i = 0; i < nEvict; i++) {
    m_queue.pop_front();
  }
  BOOST_ASSERT(m_queue.size() >= m_capacity);

  NFD_LOG_TRACE("evicted=" << nEvict << " size=" << size() << " capacity=" << m_capacity);
}

} // namespace nfd
