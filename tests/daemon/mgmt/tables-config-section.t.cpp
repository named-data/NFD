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

#include "mgmt/tables-config-section.hpp"
#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "tests/check-typeid.hpp"
#include "../fw/dummy-strategy.hpp"
#include "../fw/install-strategy.hpp"

namespace nfd {
namespace tests {

class TablesConfigSectionFixture : protected BaseFixture
{
protected:
  TablesConfigSectionFixture()
    : cs(forwarder.getCs())
    , strategyChoice(forwarder.getStrategyChoice())
    , networkRegionTable(forwarder.getNetworkRegionTable())
    , tablesConfig(forwarder)
  {
  }

  void
  runConfig(const std::string& config, bool isDryRun)
  {
    ConfigFile cf;
    tablesConfig.setConfigFile(cf);
    cf.parse(config, isDryRun, "dummy-config");
  }

protected:
  Forwarder forwarder;
  Cs& cs;
  StrategyChoice& strategyChoice;
  NetworkRegionTable& networkRegionTable;

  TablesConfigSection tablesConfig;
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

class CsUnsolicitedPolicyFixture : public TablesConfigSectionFixture
{
protected:
  class DummyUnsolicitedDataPolicy : public fw::AdmitNetworkUnsolicitedDataPolicy
  {
  };

  CsUnsolicitedPolicyFixture()
  {
    forwarder.setUnsolicitedDataPolicy(make_unique<DummyUnsolicitedDataPolicy>());
  }
};

BOOST_FIXTURE_TEST_SUITE(CsUnsolicitedPolicy, CsUnsolicitedPolicyFixture)

BOOST_AUTO_TEST_CASE(NoSection)
{
  tablesConfig.ensureConfigured();

  fw::UnsolicitedDataPolicy* currentPolicy = &forwarder.getUnsolicitedDataPolicy();
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
  fw::UnsolicitedDataPolicy* currentPolicy = &forwarder.getUnsolicitedDataPolicy();
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
  fw::UnsolicitedDataPolicy* currentPolicy = &forwarder.getUnsolicitedDataPolicy();
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
        / /localhost/nfd/strategy/test-strategy-a
        /a /localhost/nfd/strategy/test-strategy-b
      }
    }
  )CONFIG";

  install<DummyStrategy>(forwarder, "/localhost/nfd/strategy/test-strategy-a");
  install<DummyStrategy>(forwarder, "/localhost/nfd/strategy/test-strategy-b");

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  {
    fw::Strategy& rootStrategy = strategyChoice.findEffectiveStrategy("/");
    BOOST_REQUIRE_NE(rootStrategy.getName(), "/localhost/nfd/strategy/test-strategy-a");
    BOOST_REQUIRE_NE(rootStrategy.getName(), "/localhost/nfd/strategy/test-strategy-b");

    fw::Strategy& aStrategy = strategyChoice.findEffectiveStrategy("/a");
    BOOST_REQUIRE_NE(aStrategy.getName(), "/localhost/nfd/strategy/test-strategy-b");
    BOOST_REQUIRE_NE(aStrategy.getName(), "/localhost/nfd/strategy/test-strategy-a");
  }

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  {
    fw::Strategy& rootStrategy = strategyChoice.findEffectiveStrategy("/");
    BOOST_REQUIRE_EQUAL(rootStrategy.getName(), "/localhost/nfd/strategy/test-strategy-a");

    fw::Strategy& aStrategy = strategyChoice.findEffectiveStrategy("/a");
    BOOST_REQUIRE_EQUAL(aStrategy.getName(), "/localhost/nfd/strategy/test-strategy-b");
  }
}

BOOST_AUTO_TEST_CASE(Versioned)
{
  const std::string CONFIG = R"CONFIG(
    tables
    {
      strategy_choice
      {
        /test/latest /localhost/nfd/strategy/test-strategy-a
        /test/old /localhost/nfd/strategy/test-strategy-a/%FD%01
      }
    }
  )CONFIG";

  install<DummyStrategy>(forwarder, "/localhost/nfd/strategy/test-strategy-a/%FD%01");
  install<DummyStrategy>(forwarder, "/localhost/nfd/strategy/test-strategy-a/%FD%02");

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  {
    fw::Strategy& testLatestStrategy = strategyChoice.findEffectiveStrategy("/test/latest");
    BOOST_REQUIRE_NE(testLatestStrategy.getName(),
                     "/localhost/nfd/strategy/test-strategy-a/%FD%01");
    BOOST_REQUIRE_NE(testLatestStrategy.getName(),
                     "/localhost/nfd/strategy/test-strategy-a/%FD%02");

    fw::Strategy& testOldStrategy = strategyChoice.findEffectiveStrategy("/test/old");
    BOOST_REQUIRE_NE(testOldStrategy.getName(),
                     "/localhost/nfd/strategy/test-strategy-a/%FD%01");
    BOOST_REQUIRE_NE(testOldStrategy.getName(),
                     "/localhost/nfd/strategy/test-strategy-a/%FD%02");
  }

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  {
    fw::Strategy& testLatestStrategy = strategyChoice.findEffectiveStrategy("/test/latest");
    BOOST_REQUIRE_EQUAL(testLatestStrategy.getName(),
                        "/localhost/nfd/strategy/test-strategy-a/%FD%02");

    fw::Strategy& testOldStrategy = strategyChoice.findEffectiveStrategy("/test/old");
    BOOST_REQUIRE_EQUAL(testOldStrategy.getName(),
                        "/localhost/nfd/strategy/test-strategy-a/%FD%01");
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
        /localhost/nfd/strategy/test-strategy-a
      }
    }
  )CONFIG";

  install<DummyStrategy>(forwarder, "/localhost/nfd/strategy/test-strategy-a");

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
        / /localhost/nfd/strategy/test-strategy-a
        /a /localhost/nfd/strategy/test-strategy-b
        / /localhost/nfd/strategy/test-strategy-b
      }
    }
  )CONFIG";

  install<DummyStrategy>(forwarder, "/localhost/nfd/strategy/test-strategy-a");
  install<DummyStrategy>(forwarder, "/localhost/nfd/strategy/test-strategy-b");

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
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
