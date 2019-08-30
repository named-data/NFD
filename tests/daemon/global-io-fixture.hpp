/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_GLOBAL_IO_FIXTURE_HPP
#define NFD_TESTS_DAEMON_GLOBAL_IO_FIXTURE_HPP

#include "tests/clock-fixture.hpp"

namespace nfd {
namespace tests {

/** \brief A fixture providing proper setup and teardown of the global io_service.
 *
 *  Every daemon fixture or test case should inherit from this fixture,
 *  to have per test case io_service initialization and cleanup.
 */
class GlobalIoFixture
{
protected:
  GlobalIoFixture();

  ~GlobalIoFixture();

  /** \brief Poll the global io_service.
   */
  size_t
  pollIo();

protected:
  /** \brief Reference to the global io_service instance.
   */
  boost::asio::io_service& g_io;
};

/** \brief GlobalIoFixture that also overrides steady clock and system clock.
 */
class GlobalIoTimeFixture : public GlobalIoFixture, public ClockFixture
{
protected:
  GlobalIoTimeFixture();
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_GLOBAL_IO_FIXTURE_HPP
