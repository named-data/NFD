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

#include "table/dead-nonce-list.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TableDeadNonceList, BaseFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  Name nameA("ndn:/A");
  Name nameB("ndn:/B");
  const uint32_t nonce1 = 0x53b4eaa8;
  const uint32_t nonce2 = 0x1f46372b;

  DeadNonceList dnl;
  BOOST_CHECK_EQUAL(dnl.size(), 0);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce1), false);

  dnl.add(nameA, nonce1);
  BOOST_CHECK_EQUAL(dnl.size(), 1);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce1), true);
  BOOST_CHECK_EQUAL(dnl.has(nameA, nonce2), false);
  BOOST_CHECK_EQUAL(dnl.has(nameB, nonce1), false);
}

BOOST_AUTO_TEST_CASE(MinLifetime)
{
  BOOST_CHECK_THROW(DeadNonceList dnl(time::milliseconds::zero()), std::invalid_argument);
}

/// A Fixture that periodically inserts Nonces
class PeriodicalInsertionFixture : public BaseFixture
{
protected:
  PeriodicalInsertionFixture()
    : dnl(LIFETIME)
    , name("ndn:/N")
    , addNonceBatch(0)
    , addNonceInterval(LIFETIME / DeadNonceList::EXPECTED_MARK_COUNT)
  {
    this->addNonce();
  }

  ~PeriodicalInsertionFixture()
  {
    scheduler::cancel(addNonceEvent);
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

    scheduler::cancel(addNonceEvent); // avoid double schedules
    if (addNonceInterval > time::nanoseconds::zero()) {
      addNonceEvent = scheduler::schedule(addNonceInterval,
                                          bind(&PeriodicalInsertionFixture::addNonce, this));
    }
  }

protected:
  static const time::nanoseconds LIFETIME;
  DeadNonceList dnl;
  Name name;
  uint32_t lastNonce;
  size_t addNonceBatch;
  time::nanoseconds addNonceInterval;
  scheduler::EventId addNonceEvent;
};
const time::nanoseconds PeriodicalInsertionFixture::LIFETIME = time::milliseconds(200);

BOOST_FIXTURE_TEST_CASE(Lifetime, PeriodicalInsertionFixture)
{
  BOOST_CHECK_EQUAL(dnl.getLifetime(), LIFETIME);

  LimitedIo limitedIo;

  const int RATE = DeadNonceList::INITIAL_CAPACITY / 2;
  this->setRate(RATE);
  limitedIo.defer(LIFETIME * 10);

  Name nameC("ndn:/C");
  const uint32_t nonceC = 0x25390656;
  BOOST_CHECK_EQUAL(dnl.has(nameC, nonceC), false);
  dnl.add(nameC, nonceC);
  BOOST_CHECK_EQUAL(dnl.has(nameC, nonceC), true);

  limitedIo.defer(LIFETIME / 2); // -50%, entry should exist
  BOOST_CHECK_EQUAL(dnl.has(nameC, nonceC), true);

  limitedIo.defer(LIFETIME); // +50%, entry should be gone
  BOOST_CHECK_EQUAL(dnl.has(nameC, nonceC), false);
}

BOOST_FIXTURE_TEST_CASE(CapacityDown, PeriodicalInsertionFixture)
{
  LimitedIo limitedIo;

  ssize_t cap0 = dnl.m_capacity;

  const int RATE = DeadNonceList::INITIAL_CAPACITY / 3;
  this->setRate(RATE);
  limitedIo.defer(LIFETIME * 10);

  ssize_t cap1 = dnl.m_capacity;
  BOOST_CHECK_LT(std::abs(cap1 - RATE), std::abs(cap0 - RATE));
}

BOOST_FIXTURE_TEST_CASE(CapacityUp, PeriodicalInsertionFixture)
{
  LimitedIo limitedIo;

  ssize_t cap0 = dnl.m_capacity;

  const int RATE = DeadNonceList::INITIAL_CAPACITY * 3;
  this->setRate(RATE);
  limitedIo.defer(LIFETIME * 10);

  ssize_t cap1 = dnl.m_capacity;
  BOOST_CHECK_LT(std::abs(cap1 - RATE), std::abs(cap0 - RATE));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
