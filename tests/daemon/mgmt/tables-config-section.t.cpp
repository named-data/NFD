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

#include "mgmt/tables-config-section.hpp"

#include "fw/best-route-strategy.hpp"
#include "fw/forwarder.hpp"
#include "table/cs-policy-lru.hpp"
#include "table/cs-policy-priority-fifo.hpp"

#include "tests/test-common.hpp"
#include "tests/check-typeid.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/fw/dummy-strategy.hpp"

namespace nfd {
namespace tests {

class TablesConfigSectionFixture : public GlobalIoFixture
{
protected:
  TablesConfigSectionFixture()
  {
    DummyStrategy::registerAs(strategyP);
    DummyStrategy::registerAs(strategyP1);
    // strategyP1Marker is NOT registered
    DummyStrategy::registerAs(strategyQ);
  }

  void
  runConfig(const std::string& config, bool isDryRun)
  {
    ConfigFile cf;
    tablesConfig.setConfigFile(cf);
    cf.parse(config, isDryRun, "dummy-config");
  }

protected:
  FaceTable faceTable;
  Forwarder forwarder{faceTable};
  Cs& cs{forwarder.getCs()};
  StrategyChoice& strategyChoice{forwarder.getStrategyChoice()};
  NetworkRegionTable& networkRegionTable{forwarder.getNetworkRegionTable()};

  TablesConfigSection tablesConfig{forwarder};

  const Name defaultStrategy = fw::BestRouteStrategy::getStrategyName();
  const Name strategyP = Name("/tables-config-section-strategy-P").appendVersion(2);
  const Name strategyP1 = "/tables-config-section-strategy-P/v=1";
  const Name strategyP1Marker = "/tables-config-section-strategy-P/%FD%01";
  const Name strategyQ = Name("/tables-config-section-strategy-Q").appendVersion(2);
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestTablesConfigSection, TablesConfigSectionFixture)

BOOST_AUTO_TEST_SUITE(CsMaxPackets)

BOOST_AUTO_TEST_CASE(NoSection)
{
  const size_t initialLimit = cs.getLimit();

  tablesConfig.ensureConfigured();
  BOOST_CHECK_NE(cs.getLimit(), initialLimit);
}

BOOST_AUTO_TEST_CASE(Default)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
    }
  )CONFIG";

  const size_t initialLimit = cs.getLimit();

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  BOOST_CHECK_EQUAL(cs.getLimit(), initialLimit);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  BOOST_CHECK_NE(cs.getLimit(), initialLimit);
}

BOOST_AUTO_TEST_CASE(Valid)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      cs_max_packets 101
    }
  )CONFIG";

  BOOST_REQUIRE_NE(cs.getLimit(), 101);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  BOOST_CHECK_NE(cs.getLimit(), 101);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  BOOST_CHECK_EQUAL(cs.getLimit(), 101);

  tablesConfig.ensureConfigured();
  BOOST_CHECK_EQUAL(cs.getLimit(), 101);
}

BOOST_AUTO_TEST_CASE(MissingValue)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      cs_max_packets
    }
  )CONFIG";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(InvalidValue)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      cs_max_packets invalid
    }
  )CONFIG";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // CsMaxPackets

BOOST_AUTO_TEST_SUITE(CsPolicy)

BOOST_AUTO_TEST_CASE(Default)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
    }
  )CONFIG";

  runConfig(CONFIG, false);
  cs::Policy* currentPolicy = cs.getPolicy();
  NFD_CHECK_TYPEID_EQUAL(*currentPolicy, cs::LruPolicy);
}

BOOST_AUTO_TEST_CASE(Known)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      cs_policy priority_fifo
    }
  )CONFIG";

  runConfig(CONFIG, true);
  cs::Policy* currentPolicy = cs.getPolicy();
  NFD_CHECK_TYPEID_EQUAL(*currentPolicy, cs::LruPolicy);

  runConfig(CONFIG, false);
  currentPolicy = cs.getPolicy();
  NFD_CHECK_TYPEID_EQUAL(*currentPolicy, cs::PriorityFifoPolicy);
}

BOOST_AUTO_TEST_CASE(Unknown)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      cs_policy unknown
    }
  )CONFIG";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // CsPolicy

class CsUnsolicitedPolicyFixture : public TablesConfigSectionFixture
{
protected:
  CsUnsolicitedPolicyFixture()
  {
    forwarder.setUnsolicitedDataPolicy(make_unique<fw::AdmitNetworkUnsolicitedDataPolicy>());
  }
};

BOOST_FIXTURE_TEST_SUITE(CsUnsolicitedPolicy, CsUnsolicitedPolicyFixture)

BOOST_AUTO_TEST_CASE(NoSection)
{
  tablesConfig.ensureConfigured();

  auto* currentPolicy = &forwarder.getUnsolicitedDataPolicy();
  NFD_CHECK_TYPEID_EQUAL(*currentPolicy, fw::DefaultUnsolicitedDataPolicy);
}

BOOST_AUTO_TEST_CASE(Default)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
    }
  )CONFIG";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  auto* currentPolicy = &forwarder.getUnsolicitedDataPolicy();
  NFD_CHECK_TYPEID_NE(*currentPolicy, fw::DefaultUnsolicitedDataPolicy);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  currentPolicy = &forwarder.getUnsolicitedDataPolicy();
  NFD_CHECK_TYPEID_EQUAL(*currentPolicy, fw::DefaultUnsolicitedDataPolicy);
}

BOOST_AUTO_TEST_CASE(Known)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      cs_unsolicited_policy admit-all
    }
  )CONFIG";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  auto* currentPolicy = &forwarder.getUnsolicitedDataPolicy();
  NFD_CHECK_TYPEID_NE(*currentPolicy, fw::AdmitAllUnsolicitedDataPolicy);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  currentPolicy = &forwarder.getUnsolicitedDataPolicy();
  NFD_CHECK_TYPEID_EQUAL(*currentPolicy, fw::AdmitAllUnsolicitedDataPolicy);
}

BOOST_AUTO_TEST_CASE(Unknown)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      cs_unsolicited_policy unknown
    }
  )CONFIG";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // CsUnsolicitedPolicy

BOOST_AUTO_TEST_SUITE(StrategyChoice)

BOOST_AUTO_TEST_CASE(Unversioned)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      strategy_choice
      {
        / /tables-config-section-strategy-P
        /a /tables-config-section-strategy-Q
      }
    }
  )CONFIG";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  {
    fw::Strategy& rootStrategy = strategyChoice.findEffectiveStrategy("/");
    BOOST_CHECK_EQUAL(rootStrategy.getInstanceName(), defaultStrategy);

    fw::Strategy& aStrategy = strategyChoice.findEffectiveStrategy("/a");
    BOOST_CHECK_EQUAL(aStrategy.getInstanceName(), defaultStrategy);
  }

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  {
    fw::Strategy& rootStrategy = strategyChoice.findEffectiveStrategy("/");
    BOOST_CHECK_EQUAL(rootStrategy.getInstanceName(), strategyP.getPrefix(-1));
    NFD_CHECK_TYPEID_EQUAL(rootStrategy, DummyStrategy);

    fw::Strategy& aStrategy = strategyChoice.findEffectiveStrategy("/a");
    BOOST_CHECK_EQUAL(aStrategy.getInstanceName(), strategyQ.getPrefix(-1));
    NFD_CHECK_TYPEID_EQUAL(aStrategy, DummyStrategy);
  }
}

BOOST_AUTO_TEST_CASE(Versioned)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      strategy_choice
      {
        /test/latest /tables-config-section-strategy-P
        /test/old /tables-config-section-strategy-P/v=1
        /test/marker /tables-config-section-strategy-P/%FD%01
      }
    }
  )CONFIG";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  {
    fw::Strategy& testLatestStrategy = strategyChoice.findEffectiveStrategy("/test/latest");
    BOOST_CHECK_EQUAL(testLatestStrategy.getInstanceName(), defaultStrategy);

    fw::Strategy& testOldStrategy = strategyChoice.findEffectiveStrategy("/test/old");
    BOOST_CHECK_EQUAL(testOldStrategy.getInstanceName(), defaultStrategy);

    fw::Strategy& testMarkerStrategy = strategyChoice.findEffectiveStrategy("/test/marker");
    BOOST_CHECK_EQUAL(testMarkerStrategy.getInstanceName(), defaultStrategy);
  }

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  {
    fw::Strategy& testLatestStrategy = strategyChoice.findEffectiveStrategy("/test/latest");
    BOOST_CHECK_EQUAL(testLatestStrategy.getInstanceName(), strategyP.getPrefix(-1));
    NFD_CHECK_TYPEID_EQUAL(testLatestStrategy, DummyStrategy);

    fw::Strategy& testOldStrategy = strategyChoice.findEffectiveStrategy("/test/old");
    BOOST_CHECK_EQUAL(testOldStrategy.getInstanceName(), strategyP1);
    NFD_CHECK_TYPEID_EQUAL(testOldStrategy, DummyStrategy);

    fw::Strategy& testMarkerStrategy = strategyChoice.findEffectiveStrategy("/test/marker");
    BOOST_CHECK_EQUAL(testMarkerStrategy.getInstanceName(), strategyP1Marker);
    NFD_CHECK_TYPEID_EQUAL(testMarkerStrategy, DummyStrategy);
  }
}

BOOST_AUTO_TEST_CASE(NonExisting)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      strategy_choice
      {
        / /localhost/nfd/strategy/test-doesnotexist
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(MissingPrefix)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      strategy_choice
      {
        /tables-config-section-strategy-P
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(Duplicate)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      strategy_choice
      {
        / /tables-config-section-strategy-P
        /a /tables-config-section-strategy-Q
        / /tables-config-section-strategy-Q
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(UnacceptableParameters)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      strategy_choice
      {
        / /localhost/nfd/strategy/best-route/v=5/param
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(runConfig(CONFIG, true));
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // StrategyChoice

BOOST_AUTO_TEST_SUITE(NetworkRegion)

BOOST_AUTO_TEST_CASE(Basic)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      network_region
      {
        /test/regionA
        /test/regionB/component
      }
    }
  )CONFIG";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  BOOST_CHECK_EQUAL(networkRegionTable.size(), 0);

  BOOST_CHECK(networkRegionTable.find("/test/regionA") == networkRegionTable.end());
  BOOST_CHECK(networkRegionTable.find("/test/regionB/component") == networkRegionTable.end());

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  BOOST_CHECK_EQUAL(networkRegionTable.size(), 2);

  BOOST_CHECK(networkRegionTable.find("/test/regionA") != networkRegionTable.end());
  BOOST_CHECK(networkRegionTable.find("/test/regionB/component") != networkRegionTable.end());
}

BOOST_AUTO_TEST_CASE(Reload)
{
  const std::string CONFIG1 = R"CONFIG(
    tables
    {
      network_region
      {
        /some/region
      }
    }
  )CONFIG";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG1, true));
  BOOST_CHECK(networkRegionTable.find("/some/region") == networkRegionTable.end());

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG1, false));
  BOOST_CHECK(networkRegionTable.find("/some/region") != networkRegionTable.end());

  const std::string CONFIG2 = R"CONFIG(
    tables
    {
      network_region
      {
        /different/region
      }
    }
  )CONFIG";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG2, true));
  BOOST_CHECK(networkRegionTable.find("/some/region") != networkRegionTable.end());
  BOOST_CHECK(networkRegionTable.find("/different/region") == networkRegionTable.end());

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG2, false));
  BOOST_CHECK(networkRegionTable.find("/some/region") == networkRegionTable.end());
  BOOST_CHECK(networkRegionTable.find("/different/region") != networkRegionTable.end());
}

BOOST_AUTO_TEST_SUITE_END() // NetworkRegion

BOOST_AUTO_TEST_SUITE_END() // TestTablesConfigSection
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
