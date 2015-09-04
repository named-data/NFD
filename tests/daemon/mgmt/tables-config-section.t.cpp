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

#include "mgmt/tables-config-section.hpp"
#include "fw/forwarder.hpp"


#include "tests/test-common.hpp"
#include "tests/daemon/fw/dummy-strategy.hpp"

namespace nfd {
namespace tests {

class TablesConfigSectionFixture : protected BaseFixture
{
public:

  TablesConfigSectionFixture()
    : m_cs(m_forwarder.getCs())
    , m_pit(m_forwarder.getPit())
    , m_fib(m_forwarder.getFib())
    , m_strategyChoice(m_forwarder.getStrategyChoice())
    , m_measurements(m_forwarder.getMeasurements())
    , m_networkRegionTable(m_forwarder.getNetworkRegionTable())
    , m_tablesConfig(m_cs, m_pit, m_fib, m_strategyChoice, m_measurements, m_networkRegionTable)
  {
    m_tablesConfig.setConfigFile(m_config);
  }

  void
  runConfig(const std::string& CONFIG, bool isDryRun)
  {
    m_config.parse(CONFIG, isDryRun, "dummy-config");
  }

protected:
  Forwarder m_forwarder;

  Cs& m_cs;
  Pit& m_pit;
  Fib& m_fib;
  StrategyChoice& m_strategyChoice;
  Measurements& m_measurements;
  NetworkRegionTable& m_networkRegionTable;

  TablesConfigSection m_tablesConfig;
  ConfigFile m_config;
};

BOOST_FIXTURE_TEST_SUITE(Mgmt, TablesConfigSectionFixture)
BOOST_AUTO_TEST_SUITE(TestTablesConfigSection)

BOOST_AUTO_TEST_CASE(ConfigureTablesWithDefaults)
{
  const size_t initialLimit = m_cs.getLimit();

  m_tablesConfig.ensureTablesAreConfigured();
  BOOST_CHECK_NE(initialLimit, m_cs.getLimit());
}

BOOST_AUTO_TEST_CASE(EmptyTablesSection)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "}\n";

  const size_t nCsMaxPackets = m_cs.getLimit();

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  BOOST_CHECK_EQUAL(m_cs.getLimit(), nCsMaxPackets);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  BOOST_CHECK_NE(m_cs.getLimit(), nCsMaxPackets);

  const size_t defaultLimit = m_cs.getLimit();

  m_tablesConfig.ensureTablesAreConfigured();
  BOOST_CHECK_EQUAL(defaultLimit, m_cs.getLimit());
}

BOOST_AUTO_TEST_CASE(MissingTablesSection)
{
  const std::string CONFIG =
    "not_tables\n"
    "{\n"
    "  some_other_field 0\n"
    "}\n";

  ConfigFile passiveConfig(&ConfigFile::ignoreUnknownSection);

  const size_t initialLimit = m_cs.getLimit();

  passiveConfig.parse(CONFIG, true, "dummy-config");
  BOOST_REQUIRE_EQUAL(initialLimit, m_cs.getLimit());

  passiveConfig.parse(CONFIG, false, "dummy-config");
  BOOST_REQUIRE_EQUAL(initialLimit, m_cs.getLimit());

  m_tablesConfig.ensureTablesAreConfigured();
  BOOST_CHECK_NE(initialLimit, m_cs.getLimit());
}

BOOST_AUTO_TEST_SUITE(Cs)

BOOST_AUTO_TEST_CASE(ValidCsMaxPackets)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  cs_max_packets 101\n"
    "}\n";

  BOOST_REQUIRE_NE(m_cs.getLimit(), 101);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  BOOST_CHECK_NE(m_cs.getLimit(), 101);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  BOOST_CHECK_EQUAL(m_cs.getLimit(), 101);

  m_tablesConfig.ensureTablesAreConfigured();
  BOOST_CHECK_EQUAL(m_cs.getLimit(), 101);
}

BOOST_AUTO_TEST_CASE(MissingValueCsMaxPackets)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  cs_max_packets\n"
    "}\n";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(InvalidValueCsMaxPackets)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  cs_max_packets invalid\n"
    "}\n";

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // Cs

BOOST_AUTO_TEST_SUITE(ConfigStrategy)

BOOST_AUTO_TEST_CASE(Unversioned)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  strategy_choice\n"
    "  {\n"
    "    / /localhost/nfd/strategy/test-strategy-a\n"
    "    /a /localhost/nfd/strategy/test-strategy-b\n"
    "  }\n"
    "}\n";

  m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                      "/localhost/nfd/strategy/test-strategy-a"));
  m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                      "/localhost/nfd/strategy/test-strategy-b"));

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  {
    fw::Strategy& rootStrategy = m_strategyChoice.findEffectiveStrategy("/");
    BOOST_REQUIRE_NE(rootStrategy.getName(), "/localhost/nfd/strategy/test-strategy-a");
    BOOST_REQUIRE_NE(rootStrategy.getName(), "/localhost/nfd/strategy/test-strategy-b");

    fw::Strategy& aStrategy = m_strategyChoice.findEffectiveStrategy("/a");
    BOOST_REQUIRE_NE(aStrategy.getName(), "/localhost/nfd/strategy/test-strategy-b");
    BOOST_REQUIRE_NE(aStrategy.getName(), "/localhost/nfd/strategy/test-strategy-a");
  }

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  {
    fw::Strategy& rootStrategy = m_strategyChoice.findEffectiveStrategy("/");
    BOOST_REQUIRE_EQUAL(rootStrategy.getName(), "/localhost/nfd/strategy/test-strategy-a");

    fw::Strategy& aStrategy = m_strategyChoice.findEffectiveStrategy("/a");
    BOOST_REQUIRE_EQUAL(aStrategy.getName(), "/localhost/nfd/strategy/test-strategy-b");
  }
}

BOOST_AUTO_TEST_CASE(Versioned)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "strategy_choice\n"
    "{\n"
    "  /test/latest /localhost/nfd/strategy/test-strategy-a\n"
    "  /test/old /localhost/nfd/strategy/test-strategy-a/%FD%01\n"
    "}\n"
    "}\n";


  auto version1 = make_shared<DummyStrategy>(ref(m_forwarder),
                                             "/localhost/nfd/strategy/test-strategy-a/%FD%01");

  auto version2 = make_shared<DummyStrategy>(ref(m_forwarder),
                                             "/localhost/nfd/strategy/test-strategy-a/%FD%02");
  m_strategyChoice.install(version1);
  m_strategyChoice.install(version2);

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  {
    fw::Strategy& testLatestStrategy = m_strategyChoice.findEffectiveStrategy("/test/latest");
    BOOST_REQUIRE_NE(testLatestStrategy.getName(),
                     "/localhost/nfd/strategy/test-strategy-a/%FD%01");
    BOOST_REQUIRE_NE(testLatestStrategy.getName(),
                     "/localhost/nfd/strategy/test-strategy-a/%FD%02");

    fw::Strategy& testOldStrategy = m_strategyChoice.findEffectiveStrategy("/test/old");
    BOOST_REQUIRE_NE(testOldStrategy.getName(),
                     "/localhost/nfd/strategy/test-strategy-a/%FD%01");
    BOOST_REQUIRE_NE(testOldStrategy.getName(),
                     "/localhost/nfd/strategy/test-strategy-a/%FD%02");
  }

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  {
    fw::Strategy& testLatestStrategy = m_strategyChoice.findEffectiveStrategy("/test/latest");
    BOOST_REQUIRE_EQUAL(testLatestStrategy.getName(),
                        "/localhost/nfd/strategy/test-strategy-a/%FD%02");

    fw::Strategy& testOldStrategy = m_strategyChoice.findEffectiveStrategy("/test/old");
    BOOST_REQUIRE_EQUAL(testOldStrategy.getName(),
                        "/localhost/nfd/strategy/test-strategy-a/%FD%01");
  }
}

BOOST_AUTO_TEST_CASE(NonExisting)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  strategy_choice\n"
    "  {\n"
    "    / /localhost/nfd/strategy/test-doesnotexist\n"
    "  }\n"
    "}\n";


  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(MissingPrefix)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  strategy_choice\n"
    "  {\n"
    "    /localhost/nfd/strategy/test-strategy-a\n"
    "  }\n"
    "}\n";


  m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                      "/localhost/nfd/strategy/test-strategy-a"));

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(Duplicate)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  strategy_choice\n"
    "  {\n"
    "    / /localhost/nfd/strategy/test-strategy-a\n"
    "    /a /localhost/nfd/strategy/test-strategy-b\n"
    "    / /localhost/nfd/strategy/test-strategy-b\n"
    "  }\n"
    "}\n";

  m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                      "/localhost/nfd/strategy/test-strategy-a"));
  m_strategyChoice.install(make_shared<DummyStrategy>(ref(m_forwarder),
                                                      "/localhost/nfd/strategy/test-strategy-b"));

  BOOST_CHECK_THROW(runConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(runConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // StrategyChoice

BOOST_AUTO_TEST_SUITE(NetworkRegion)

BOOST_AUTO_TEST_CASE(Basic)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  network_region\n"
    "  {\n"
    "    /test/regionA\n"
    "    /test/regionB/component\n"
    "  }\n"
    "}\n";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, true));
  BOOST_CHECK_EQUAL(m_networkRegionTable.size(), 0);

  BOOST_CHECK(m_networkRegionTable.find("/test/regionA") == m_networkRegionTable.end());
  BOOST_CHECK(m_networkRegionTable.find("/test/regionB/component") == m_networkRegionTable.end());

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG, false));
  BOOST_CHECK_EQUAL(m_networkRegionTable.size(), 2);

  BOOST_CHECK(m_networkRegionTable.find("/test/regionA") != m_networkRegionTable.end());
  BOOST_CHECK(m_networkRegionTable.find("/test/regionB/component") != m_networkRegionTable.end());
}

BOOST_AUTO_TEST_CASE(Reload)
{
  const std::string CONFIG1 =
    "tables\n"
    "{\n"
    "  network_region\n"
    "  {\n"
    "    /some/region\n"
    "  }\n"
    "}\n";

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG1, true));
  BOOST_CHECK(m_networkRegionTable.find("/some/region") == m_networkRegionTable.end());

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG1, false));
  BOOST_CHECK(m_networkRegionTable.find("/some/region") != m_networkRegionTable.end());

  const std::string CONFIG2 =
    "tables\n"
    "{\n"
    "  network_region\n"
    "  {\n"
    "    /different/region\n"
    "  }\n"
    "}\n";
  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG2, true));
  BOOST_CHECK(m_networkRegionTable.find("/some/region") != m_networkRegionTable.end());
  BOOST_CHECK(m_networkRegionTable.find("/different/region") == m_networkRegionTable.end());

  BOOST_REQUIRE_NO_THROW(runConfig(CONFIG2, false));
  BOOST_CHECK(m_networkRegionTable.find("/some/region") == m_networkRegionTable.end());
  BOOST_CHECK(m_networkRegionTable.find("/different/region") != m_networkRegionTable.end());
}

BOOST_AUTO_TEST_SUITE_END() // NetworkRegion

BOOST_AUTO_TEST_SUITE_END() // TestTableConfigSection
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
