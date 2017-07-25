/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#ifndef NFD_TESTS_TEST_COMMON_HPP
#define NFD_TESTS_TEST_COMMON_HPP

#include "boost-test.hpp"

#include "core/global-io.hpp"
#include "core/logger.hpp"

#include <ndn-cxx/util/time-unit-test-clock.hpp>

#ifdef HAVE_PRIVILEGE_DROP_AND_ELEVATE
#include <unistd.h>
#define SKIP_IF_NOT_SUPERUSER() \
  do { \
    if (::geteuid() != 0) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require superuser privileges"); \
      return; \
    } \
  } while (false)
#else
#define SKIP_IF_NOT_SUPERUSER()
#endif // HAVE_PRIVILEGE_DROP_AND_ELEVATE

namespace nfd {
namespace tests {

/** \brief base test fixture
 *
 *  Every test case should be based on this fixture,
 *  to have per test case io_service initialization.
 */
class BaseFixture
{
protected:
  BaseFixture();

  ~BaseFixture();

protected:
  /** \brief reference to global io_service
   */
  boost::asio::io_service& g_io;
};

/** \brief a base test fixture that overrides steady clock and system clock
 */
class UnitTestTimeFixture : public virtual BaseFixture
{
protected:
  UnitTestTimeFixture();

  ~UnitTestTimeFixture();

  /** \brief advance steady and system clocks
   *
   *  Clocks are advanced in increments of \p tick for \p nTicks ticks.
   *  After each tick, global io_service is polled to process pending I/O events.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancing would stop in case of an exception.
   */
  void
  advanceClocks(const time::nanoseconds& tick, size_t nTicks = 1);

  /** \brief advance steady and system clocks
   *
   *  Clocks are advanced in increments of \p tick for \p total time.
   *  The last increment might be shorter than \p tick.
   *  After each tick, global io_service is polled to process pending I/O events.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancing would stop in case of an exception.
   */
  void
  advanceClocks(const time::nanoseconds& tick, const time::nanoseconds& total);

protected:
  shared_ptr<time::UnitTestSteadyClock> steadyClock;
  shared_ptr<time::UnitTestSystemClock> systemClock;

  friend class LimitedIo;
};

/** \brief create an Interest
 *  \param name Interest name
 *  \param nonce if non-zero, set Nonce to this value
 *               (useful for creating Nack with same Nonce)
 */
shared_ptr<Interest>
makeInterest(const Name& name, uint32_t nonce = 0);

/** \brief create a Data with fake signature
 *  \note Data may be modified afterwards without losing the fake signature.
 *        If a real signature is desired, sign again with KeyChain.
 */
shared_ptr<Data>
makeData(const Name& name);

/** \brief add a fake signature to Data
 */
Data&
signData(Data& data);

/** \brief add a fake signature to Data
 */
inline shared_ptr<Data>
signData(shared_ptr<Data> data)
{
  signData(*data);
  return data;
}

/** \brief create a Nack
 *  \param interest Interest
 *  \param reason Nack reason
 */
lp::Nack
makeNack(Interest interest, lp::NackReason reason);

/** \brief create a Nack
 *  \param name Interest name
 *  \param nonce Interest nonce
 *  \param reason Nack reason
 */
lp::Nack
makeNack(const Name& name, uint32_t nonce, lp::NackReason reason);

/** \brief replace a name component
 *  \param[inout] name name
 *  \param index name component index
 *  \param a arguments to name::Component constructor
 */
template<typename...A>
void
setNameComponent(Name& name, ssize_t index, const A& ...a)
{
  Name name2 = name.getPrefix(index);
  name2.append(name::Component(a...));
  name2.append(name.getSubName(name2.size()));
  name = name2;
}

template<typename Packet, typename...A>
void
setNameComponent(Packet& packet, ssize_t index, const A& ...a)
{
  Name name = packet.getName();
  setNameComponent(name, index, a...);
  packet.setName(name);
}

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_TEST_COMMON_HPP
