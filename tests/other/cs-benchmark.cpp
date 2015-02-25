/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "table/cs.hpp"
#include <ndn-cxx/security/key-chain.hpp>

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

class CsBenchmarkFixture : public BaseFixture
{
protected:
  CsBenchmarkFixture()
  {
#ifdef _DEBUG
    BOOST_TEST_MESSAGE("Benchmark compiled in debug mode is unreliable, "
                       "please compile in release mode.");
#endif // _DEBUG

    cs.setLimit(CS_CAPACITY);
  }

  time::microseconds
  timedRun(std::function<void()> f)
  {
    time::steady_clock::TimePoint t1 = time::steady_clock::now();
    f();
    time::steady_clock::TimePoint t2 = time::steady_clock::now();
    return time::duration_cast<time::microseconds>(t2 - t1);
  }

  void
  find(const Interest& interest)
  {
    cs.find(interest, bind([]{}), bind([]{}));
  }

protected:
  typedef std::function<Name(size_t)> NameGenerator;

  class SimpleNameGenerator
  {
  public:
    explicit
    SimpleNameGenerator(const Name& prefix = "/cs/benchmark")
      : m_prefix(prefix)
    {
    }

    Name
    operator()(size_t i) const
    {
      Name name(m_prefix);
      name.appendNumber(i % 4);
      name.appendNumber(i);
      return name;
    }

  private:
    Name m_prefix;
  };

  std::vector<shared_ptr<Interest>>
  makeInterestWorkload(size_t count, const NameGenerator& genName = SimpleNameGenerator())
  {
    std::vector<shared_ptr<Interest>> workload(count);
    for (size_t i = 0; i < count; ++i) {
      Name name = genName(i);
      workload[i] = makeInterest(name);
    }
    return workload;
  }

  std::vector<shared_ptr<Data>>
  makeDataWorkload(size_t count, const NameGenerator& genName = SimpleNameGenerator())
  {
    std::vector<shared_ptr<Data>> workload(count);
    for (size_t i = 0; i < count; ++i) {
      Name name = genName(i);
      workload[i] = makeData(name);
    }
    return workload;
  }

protected:
  Cs cs;
  static const size_t CS_CAPACITY = 50000;
};

BOOST_FIXTURE_TEST_SUITE(TableCsBenchmark, CsBenchmarkFixture)

// find miss, then insert
BOOST_AUTO_TEST_CASE(FindMissInsert)
{
  const size_t N_WORKLOAD = CS_CAPACITY * 2;
  const size_t REPEAT = 4;

  std::vector<shared_ptr<Interest>> interestWorkload = makeInterestWorkload(N_WORKLOAD);
  std::vector<shared_ptr<Data>> dataWorkload[REPEAT];
  for (size_t j = 0; j < REPEAT; ++j) {
    dataWorkload[j] = makeDataWorkload(N_WORKLOAD);
  }

  time::microseconds d = timedRun([&] {
    for (size_t j = 0; j < REPEAT; ++j) {
      for (size_t i = 0; i < N_WORKLOAD; ++i) {
        find(*interestWorkload[i]);
        cs.insert(*dataWorkload[j][i], false);
      }
    }
  });
  BOOST_TEST_MESSAGE("find(miss)-insert " << (N_WORKLOAD * REPEAT) << ": " << d);
}

// insert, then find hit
BOOST_AUTO_TEST_CASE(InsertFindHit)
{
  const size_t N_WORKLOAD = CS_CAPACITY * 2;
  const size_t REPEAT = 4;

  std::vector<shared_ptr<Interest>> interestWorkload = makeInterestWorkload(N_WORKLOAD);
  std::vector<shared_ptr<Data>> dataWorkload[REPEAT];
  for (size_t j = 0; j < REPEAT; ++j) {
    dataWorkload[j] = makeDataWorkload(N_WORKLOAD);
  }

  time::microseconds d = timedRun([&] {
    for (size_t j = 0; j < REPEAT; ++j) {
      for (size_t i = 0; i < N_WORKLOAD; ++i) {
        cs.insert(*dataWorkload[j][i], false);
        find(*interestWorkload[i]);
      }
    }
  });
  BOOST_TEST_MESSAGE("insert-find(hit) " << (N_WORKLOAD * REPEAT) << ": " << d);
}

// find(leftmost) hit
BOOST_AUTO_TEST_CASE(Leftmost)
{
  const size_t N_CHILDREN = 10;
  const size_t N_INTERESTS = CS_CAPACITY / N_CHILDREN;
  std::vector<shared_ptr<Interest>> interestWorkload = makeInterestWorkload(N_INTERESTS);
  for (auto&& interest : interestWorkload) {
    interest->setChildSelector(0);
    for (size_t j = 0; j < N_CHILDREN; ++j) {
      Name name = interest->getName();
      name.appendNumber(j);
      shared_ptr<Data> data = makeData(name);
      cs.insert(*data, false);
    }
  }
  BOOST_REQUIRE(cs.size() == N_INTERESTS * N_CHILDREN);

  const size_t REPEAT = 4;

  time::microseconds d = timedRun([&] {
    for (size_t j = 0; j < REPEAT; ++j) {
      for (const auto& interest : interestWorkload) {
        find(*interest);
      }
    }
  });
  BOOST_TEST_MESSAGE("find(leftmost) " << (N_INTERESTS * N_CHILDREN * REPEAT) << ": " << d);
}

// find(rightmost) hit
BOOST_AUTO_TEST_CASE(Rightmost)
{
  const size_t N_CHILDREN = 10;
  const size_t N_INTERESTS = CS_CAPACITY / N_CHILDREN;
  std::vector<shared_ptr<Interest>> interestWorkload = makeInterestWorkload(N_INTERESTS);
  for (auto&& interest : interestWorkload) {
    interest->setChildSelector(1);
    for (size_t j = 0; j < N_CHILDREN; ++j) {
      Name name = interest->getName();
      name.appendNumber(j);
      shared_ptr<Data> data = makeData(name);
      cs.insert(*data, false);
    }
  }
  BOOST_REQUIRE(cs.size() == N_INTERESTS * N_CHILDREN);

  const size_t REPEAT = 4;

  time::microseconds d = timedRun([&] {
    for (size_t j = 0; j < REPEAT; ++j) {
      for (const auto& interest : interestWorkload) {
        find(*interest);
      }
    }
  });
  BOOST_TEST_MESSAGE("find(rightmost) " << (N_INTERESTS * N_CHILDREN * REPEAT) << ": " << d);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
