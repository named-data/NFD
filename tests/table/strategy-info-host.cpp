/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/strategy-info-host.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

static int g_DummyStrategyInfo_count = 0;

class DummyStrategyInfo : public fw::StrategyInfo
{
public:
  DummyStrategyInfo(int id)
    : m_id(id)
  {
    ++g_DummyStrategyInfo_count;
  }
  
  virtual ~DummyStrategyInfo()
  {
    --g_DummyStrategyInfo_count;
  }
  
  int m_id;
};

BOOST_AUTO_TEST_SUITE(TableStrategyInfoHost)

BOOST_AUTO_TEST_CASE(SetGetClear)
{
  StrategyInfoHost host;
  
  BOOST_CHECK(!static_cast<bool>(host.getStrategyInfo<DummyStrategyInfo>()));
  
  g_DummyStrategyInfo_count = 0;
  
  shared_ptr<DummyStrategyInfo> info = make_shared<DummyStrategyInfo>(7591);
  host.setStrategyInfo(info);
  BOOST_REQUIRE(static_cast<bool>(host.getStrategyInfo<DummyStrategyInfo>()));
  BOOST_CHECK_EQUAL(host.getStrategyInfo<DummyStrategyInfo>()->m_id, 7591);

  info.reset(); // unlink local reference
  // host should still have a reference to info
  BOOST_REQUIRE(static_cast<bool>(host.getStrategyInfo<DummyStrategyInfo>()));
  BOOST_CHECK_EQUAL(host.getStrategyInfo<DummyStrategyInfo>()->m_id, 7591);
  
  host.clearStrategyInfo();
  BOOST_CHECK(!static_cast<bool>(host.getStrategyInfo<DummyStrategyInfo>()));
  BOOST_CHECK_EQUAL(g_DummyStrategyInfo_count, 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
