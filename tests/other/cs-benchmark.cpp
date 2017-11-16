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
#include "table/cs.hpp"

#include <ndn-cxx/security/signature-sha256-with-rsa.hpp>

#include <iostream>

#ifdef HAVE_VALGRIND
#include <valgrind/callgrind.h>
#endif

namespace nfd {
namespace tests {

class CsBenchmarkFixture
{
protected:
  CsBenchmarkFixture()
  {
#ifdef _DEBUG
    std::cerr << "Benchmark compiled in debug mode is unreliable, please compile in release mode.\n";
#endif

    cs.setLimit(CS_CAPACITY);
  }

  static time::microseconds
  timedRun(const std::function<void()>& f)
  {
#ifdef HAVE_VALGRIND
    CALLGRIND_START_INSTRUMENTATION;
#endif

    auto t1 = time::steady_clock::now();
    f();
    auto t2 = time::steady_clock::now();

#ifdef HAVE_VALGRIND
    CALLGRIND_STOP_INSTRUMENTATION;
#endif

    return time::duration_cast<time::microseconds>(t2 - t1);
  }

  static shared_ptr<Data>
  makeData(const Name& name)
  {
    auto data = make_shared<Data>(name);
    ndn::SignatureSha256WithRsa fakeSignature;
    fakeSignature.setValue(ndn::encoding::makeEmptyBlock(tlv::SignatureValue));
    data->setSignature(fakeSignature);
    data->wireEncode();
    return data;
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

  static std::vector<shared_ptr<Interest>>
  makeInterestWorkload(size_t count, const NameGenerator& genName = SimpleNameGenerator())
  {
    std::vector<shared_ptr<Interest>> workload(count);
    for (size_t i = 0; i < count; ++i) {
      Name name = genName(i);
      workload[i] = make_shared<Interest>(name);
    }
    return workload;
  }

  static std::vector<shared_ptr<Data>>
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
  static constexpr size_t CS_CAPACITY = 50000;
};

// find miss, then insert
BOOST_FIXTURE_TEST_CASE(FindMissInsert, CsBenchmarkFixture)
{
  constexpr size_t N_WORKLOAD = CS_CAPACITY * 2;
  constexpr size_t REPEAT = 4;

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

  std::cout << "find(miss)-insert " << (N_WORKLOAD * REPEAT) << ": " << d << std::endl;
}

// insert, then find hit
BOOST_FIXTURE_TEST_CASE(InsertFindHit, CsBenchmarkFixture)
{
  constexpr size_t N_WORKLOAD = CS_CAPACITY * 2;
  constexpr size_t REPEAT = 4;

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

  std::cout << "insert-find(hit) " << (N_WORKLOAD * REPEAT) << ": " << d << std::endl;
}

// find(leftmost) hit
BOOST_FIXTURE_TEST_CASE(Leftmost, CsBenchmarkFixture)
{
  constexpr size_t N_CHILDREN = 10;
  constexpr size_t N_INTERESTS = CS_CAPACITY / N_CHILDREN;
  constexpr size_t REPEAT = 4;

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

  time::microseconds d = timedRun([&] {
    for (size_t j = 0; j < REPEAT; ++j) {
      for (const auto& interest : interestWorkload) {
        find(*interest);
      }
    }
  });

  std::cout << "find(leftmost) " << (N_INTERESTS * N_CHILDREN * REPEAT) << ": " << d << std::endl;
}

// find(rightmost) hit
BOOST_FIXTURE_TEST_CASE(Rightmost, CsBenchmarkFixture)
{
  constexpr size_t N_CHILDREN = 10;
  constexpr size_t N_INTERESTS = CS_CAPACITY / N_CHILDREN;
  constexpr size_t REPEAT = 4;

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

  time::microseconds d = timedRun([&] {
    for (size_t j = 0; j < REPEAT; ++j) {
      for (const auto& interest : interestWorkload) {
        find(*interest);
      }
    }
  });

  std::cout << "find(rightmost) " << (N_INTERESTS * N_CHILDREN * REPEAT) << ": " << d << std::endl;
}

} // namespace tests
} // namespace nfd
