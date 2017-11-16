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

#include "benchmark-helpers.hpp"
#include "table/fib.hpp"
#include "table/pit.hpp"

#include <iostream>

#ifdef HAVE_VALGRIND
#include <valgrind/callgrind.h>
#endif

namespace nfd {
namespace tests {

class PitFibBenchmarkFixture
{
protected:
  PitFibBenchmarkFixture()
    : m_fib(m_nameTree)
    , m_pit(m_nameTree)
  {
#ifdef _DEBUG
    std::cerr << "Benchmark compiled in debug mode is unreliable, please compile in release mode.\n";
#endif
  }

  void
  generatePacketsAndPopulateFib(size_t nPackets,
                                size_t nFibEntries,
                                size_t fibPrefixLength,
                                size_t interestNameLength,
                                size_t dataNameLength)
  {
    BOOST_ASSERT(1 <= fibPrefixLength);
    BOOST_ASSERT(fibPrefixLength <= interestNameLength);
    BOOST_ASSERT(interestNameLength <= dataNameLength);

    for (size_t i = 0; i < nPackets; i++) {
      Name prefix(to_string(i / nFibEntries));
      extendName(prefix, fibPrefixLength);
      m_fib.insert(prefix);

      Name interestName = prefix;
      if (nPackets > nFibEntries) {
        interestName.append(to_string(i));
      }
      extendName(interestName, interestNameLength);
      interests.push_back(make_shared<Interest>(interestName));

      Name dataName = interestName;
      extendName(dataName, dataNameLength);
      data.push_back(make_shared<Data>(dataName));
    }
  }

private:
  static void
  extendName(Name& name, size_t length)
  {
    BOOST_ASSERT(length >= name.size());
    for (size_t i = name.size(); i < length; i++) {
      name.append("dup");
    }
  }

protected:
  std::vector<shared_ptr<Interest>> interests;
  std::vector<shared_ptr<Data>> data;
  std::vector<shared_ptr<pit::Entry>> pitEntries;

protected:
  NameTree m_nameTree;
  Fib m_fib;
  Pit m_pit;
};

// This test case models PIT and FIB operations with simple Interest-Data exchanges.
// A total of nRoundTrip Interests are received and forwarded, and the same number of Data are returned.
BOOST_FIXTURE_TEST_CASE(SimpleExchanges, PitFibBenchmarkFixture)
{
  // number of Interest-Data exchanges
  const size_t nRoundTrip = 1000000;
  // number of iterations between processing incoming Interest and processing incoming Data
  const size_t gap3 = 20000;
  // number of iterations between processing incoming Data and deleting PIT entry
  const size_t gap4 = 30000;
  // total amount of FIB entires
  // packet names are homogeneously extended from these FIB entries
  const size_t nFibEntries = 2000;
  // length of fibPrefix, must be >= 1
  const size_t fibPrefixLength = 1;
  // length of Interest Name >= fibPrefixLength
  const size_t interestNameLength= 2;
  // length of Data Name >= Interest Name
  const size_t dataNameLength = 3;

  generatePacketsAndPopulateFib(nRoundTrip, nFibEntries, fibPrefixLength,
                                interestNameLength, dataNameLength);

#ifdef HAVE_VALGRIND
  CALLGRIND_START_INSTRUMENTATION;
#endif

  auto t1 = time::steady_clock::now();

  for (size_t i = 0; i < nRoundTrip + gap3 + gap4; ++i) {
    if (i < nRoundTrip) {
      // process incoming Interest
      shared_ptr<pit::Entry> pitEntry = m_pit.insert(*interests[i]).first;
      pitEntries.push_back(pitEntry);
      m_fib.findLongestPrefixMatch(*pitEntry);
    }
    if (i >= gap3 && i < nRoundTrip + gap3) {
      // process incoming Data
      m_pit.findAllDataMatches(*data[i - gap3]);
    }
    if (i >= gap3 + gap4) {
      // delete PIT entry
      m_pit.erase(pitEntries[i - gap3 - gap4].get());
    }
  }

  auto t2 = time::steady_clock::now();

#ifdef HAVE_VALGRIND
  CALLGRIND_STOP_INSTRUMENTATION;
#endif

  std::cout << time::duration_cast<time::microseconds>(t2 - t1) << std::endl;
}

} // namespace tests
} // namespace nfd
