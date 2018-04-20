/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "logger.hpp"

#ifdef HAVE_CUSTOM_LOGGER
#error "This file should not be compiled when custom logger is used"
#endif

#include <cinttypes>
#include <cstdlib>
#include <stdio.h>
#include <type_traits>

#include <ndn-cxx/util/time.hpp>

namespace nfd {

Logger::Logger(const std::string& name, LogLevel level)
  : m_moduleName(name)
  , m_enabledLogLevel(level)
{
}

std::ostream&
operator<<(std::ostream& os, const LoggerTimestamp&)
{
  using namespace ndn::time;

  const auto sinceEpoch = system_clock::now().time_since_epoch();
  BOOST_ASSERT(sinceEpoch.count() >= 0);
  // use abs() to silence truncation warning in snprintf(), see #4365
  const auto usecs = std::abs(duration_cast<microseconds>(sinceEpoch).count());
  const auto usecsPerSec = microseconds::period::den;

  // 10 (whole seconds) + '.' + 6 (fraction) + '\0'
  char buffer[10 + 1 + 6 + 1];
  BOOST_ASSERT_MSG(usecs / usecsPerSec <= 9999999999, "whole seconds cannot fit in 10 characters");

  static_assert(std::is_same<microseconds::rep, int_least64_t>::value,
                "PRIdLEAST64 is incompatible with microseconds::rep");
  // std::snprintf unavailable on some platforms, see #2299
  ::snprintf(buffer, sizeof(buffer), "%" PRIdLEAST64 ".%06" PRIdLEAST64,
             usecs / usecsPerSec, usecs % usecsPerSec);

  return os << buffer;
}

} // namespace nfd
