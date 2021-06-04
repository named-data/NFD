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

#include "table/dead-nonce-list.hpp"
#include "common/global.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestDeadNonceList, GlobalIoFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  Name nameA("ndn:/A");
  Name nameB("ndn:/B");
  const Interest::Nonce nonce1(0x53b4eaa8);
  const Interest::Nonce nonce2(0x1f46372b);

  DeadNonceList dnl;
  BOOST_CHECK_EQUAL(dnl.size(), 0);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce1), false);

  dnl.add(nameA, nonce1);
  BOOST_CHECK_EQUAL(dnl.size(), 1);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce1), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce2), false);
  BOOST_CHECK_EQUAL(dnl.has(nameB, nonce1), false);
}

BOOST_AUTO_TEST_CASE(NoDuplicates)
{
  Name nameA("ndn:/A");
  const Interest::Nonce nonce1(0x53b4eaa8);
  const Interest::Nonce nonce2(0x63b4eaa8);
  const Interest::Nonce nonce3(0x73b4eaa8);
  const Interest::Nonce nonce4(0x83b4eaa8);
  const Interest::Nonce nonce5(0x93b4eaa8);

  DeadNonceList dnl;
  BOOST_CHECK_EQUAL(dnl.size(), 0);

  dnl.m_capacity = 4;
  dnl.add(nameA, nonce1);
  dnl.add(nameA, nonce2);
  dnl.add(nameA, nonce3);
  dnl.add(nameA, nonce4);
  BOOST_CHECK_EQUAL(dnl.size(), 4);

  dnl.add(nameA, nonce1);
  BOOST_CHECK_EQUAL(dnl.size(), 4);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce1), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce2), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce3), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce4), true);

  dnl.add(nameA, nonce5);
  BOOST_CHECK_EQUAL(dnl.size(), 4);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce1), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce2), false);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce3), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce4), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce5), true);

  dnl.add(nameA, nonce5);
  BOOST_CHECK_EQUAL(dnl.size(), 4);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce1), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce2), false);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce3), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce4), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce5), true);
}

BOOST_AUTO_TEST_CASE(MinLifetime)
{
  BOOST_CHECK_THROW(DeadNonceList(0_ms), std::invalid_argument);
}

/// A fixture that periodically inserts Nonces
class PeriodicalInsertionFixture : public GlobalIoTimeFixture
{
protected:
  PeriodicalInsertionFixture()
  {
    addNonce();
  }

  void
  setRate(size_t nNoncesPerLifetime)
  {
    addNonceBatch = nNoncesPerLifetime / DeadNonceList::EXPECTED_MARK_COUNT;
  }

  void
  addNonce()
  {
    for (size_t i = 0; i < addNonceBatch; ++i) {
      dnl.add(name, ++lastNonce);
    }

    addNonceEvent = getScheduler().schedule(ADD_INTERVAL, [this] { addNonce(); });
  }

  /** \brief advance clocks by LIFETIME*t
   */
  void
  advanceClocksByLifetime(double t)
  {
    advanceClocks(ADD_INTERVAL / 2, time::duration_cast<time::nanoseconds>(LIFETIME * t));
  }

protected:
  static constexpr time::nanoseconds LIFETIME = 200_ms;
  static constexpr time::nanoseconds ADD_INTERVAL = LIFETIME / DeadNonceList::EXPECTED_MARK_COUNT;

  DeadNonceList dnl{LIFETIME};
  Name name = "/N";
  uint32_t lastNonce = 0;
  size_t addNonceBatch = 0;
  scheduler::ScopedEventId addNonceEvent;
};

const time::nanoseconds PeriodicalInsertionFixture::LIFETIME;
const time::nanoseconds PeriodicalInsertionFixture::ADD_INTERVAL;

BOOST_FIXTURE_TEST_CASE(Lifetime, PeriodicalInsertionFixture)
{
  BOOST_CHECK_EQUAL(dnl.getLifetime(), LIFETIME);

  const int RATE = DeadNonceList::INITIAL_CAPACITY / 2;
  this->setRate(RATE);
  this->advanceClocksByLifetime(10.0);

  Name nameC("ndn:/C");
  const Interest::Nonce nonceC(0x25390656);
  BOOST_CHECK_EQUAL(dnl.has(nameC, nonceC), false);
  dnl.add(nameC, nonceC);
  BOOST_CHECK_EQUAL(dnl.has(nameC, nonceC), true);

  this->advanceClocksByLifetime(0.5); // -50%, entry should exist
  BOOST_CHECK_EQUAL(dnl.has(nameC, nonceC), true);

  this->advanceClocksByLifetime(1.0); // +50%, entry should be gone
  BOOST_CHECK_EQUAL(dnl.has(nameC, nonceC), false);
}

BOOST_FIXTURE_TEST_CASE(CapacityDown, PeriodicalInsertionFixture)
{
  ssize_t cap0 = dnl.m_capacity;

  const int RATE = DeadNonceList::INITIAL_CAPACITY / 3;
  this->setRate(RATE);
  this->advanceClocksByLifetime(10.0);

  ssize_t cap1 = dnl.m_capacity;
  BOOST_CHECK_LT(std::abs(cap1 - RATE), std::abs(cap0 - RATE));
}

BOOST_FIXTURE_TEST_CASE(CapacityUp, PeriodicalInsertionFixture)
{
  ssize_t cap0 = dnl.m_capacity;

  const int RATE = DeadNonceList::INITIAL_CAPACITY * 3;
  this->setRate(RATE);
  this->advanceClocksByLifetime(10.0);

  ssize_t cap1 = dnl.m_capacity;
  BOOST_CHECK_LT(std::abs(cap1 - RATE), std::abs(cap0 - RATE));
}

BOOST_AUTO_TEST_SUITE_END() // TestDeadNonceList
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace nfd
