/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "table/strategy-info-host.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

using fw::StrategyInfo;

static int g_DummyStrategyInfo_count = 0;

class DummyStrategyInfo : public StrategyInfo, noncopyable
{
public:
  static constexpr int
  getTypeId()
  {
    return 1;
  }

  DummyStrategyInfo(int id)
    : m_id(id)
  {
    ++g_DummyStrategyInfo_count;
  }

  virtual
  ~DummyStrategyInfo()
  {
    --g_DummyStrategyInfo_count;
  }

public:
  int m_id;
};

class DummyStrategyInfo2 : public StrategyInfo, noncopyable
{
public:
  static constexpr int
  getTypeId()
  {
    return 2;
  }

  DummyStrategyInfo2(int id)
    : m_id(id)
  {
  }

public:
  int m_id;
};

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestStrategyInfoHost, BaseFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  StrategyInfoHost host;
  g_DummyStrategyInfo_count = 0;
  bool isNew = false;

  DummyStrategyInfo* info = nullptr;
  std::tie(info, isNew) = host.insertStrategyInfo<DummyStrategyInfo>(3503);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK_EQUAL(g_DummyStrategyInfo_count, 1);
  BOOST_REQUIRE(info != nullptr);
  BOOST_CHECK_EQUAL(info->m_id, 3503);
  BOOST_CHECK_EQUAL(host.getStrategyInfo<DummyStrategyInfo>(), info);

  DummyStrategyInfo* info2 = nullptr;
  std::tie(info2, isNew) = host.insertStrategyInfo<DummyStrategyInfo>(1032);
  BOOST_CHECK_EQUAL(isNew, false);
  BOOST_CHECK_EQUAL(g_DummyStrategyInfo_count, 1);
  BOOST_CHECK_EQUAL(info2, info);
  BOOST_CHECK_EQUAL(info->m_id, 3503);
  BOOST_CHECK_EQUAL(host.getStrategyInfo<DummyStrategyInfo>(), info);

  host.clearStrategyInfo();
  BOOST_CHECK(host.getStrategyInfo<DummyStrategyInfo>() == nullptr);
  BOOST_CHECK_EQUAL(g_DummyStrategyInfo_count, 0);
}

BOOST_AUTO_TEST_CASE(Types)
{
  StrategyInfoHost host;
  g_DummyStrategyInfo_count = 0;

  host.insertStrategyInfo<DummyStrategyInfo>(8063);
  BOOST_REQUIRE(host.getStrategyInfo<DummyStrategyInfo>() != nullptr);
  BOOST_CHECK_EQUAL(host.getStrategyInfo<DummyStrategyInfo>()->m_id, 8063);

  host.insertStrategyInfo<DummyStrategyInfo2>(2871);
  BOOST_REQUIRE(host.getStrategyInfo<DummyStrategyInfo2>() != nullptr);
  BOOST_CHECK_EQUAL(host.getStrategyInfo<DummyStrategyInfo2>()->m_id, 2871);

  BOOST_REQUIRE(host.getStrategyInfo<DummyStrategyInfo>() != nullptr);
  BOOST_CHECK_EQUAL(host.getStrategyInfo<DummyStrategyInfo>()->m_id, 8063);

  BOOST_CHECK_EQUAL(host.eraseStrategyInfo<DummyStrategyInfo>(), 1);
  BOOST_CHECK(host.getStrategyInfo<DummyStrategyInfo>() == nullptr);
  BOOST_CHECK_EQUAL(g_DummyStrategyInfo_count, 0);
  BOOST_CHECK(host.getStrategyInfo<DummyStrategyInfo2>() != nullptr);

  BOOST_CHECK_EQUAL(host.eraseStrategyInfo<DummyStrategyInfo>(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyInfoHost
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace nfd
