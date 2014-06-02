/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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
    , m_tablesConfig(m_cs, m_pit, m_fib, m_strategyChoice, m_measurements)
  {
    m_tablesConfig.setConfigFile(m_config);
  }

  void
  runConfig(const std::string& CONFIG, bool isDryRun)
  {
    m_config.parse(CONFIG, isDryRun, "dummy-config");
  }

  bool
  validateException(const ConfigFile::Error& exception, const std::string& expectedMsg)
  {
    return exception.what() == expectedMsg;
  }

protected:
  Forwarder m_forwarder;

  Cs& m_cs;
  Pit& m_pit;
  Fib& m_fib;
  StrategyChoice& m_strategyChoice;
  Measurements& m_measurements;

  TablesConfigSection m_tablesConfig;
  ConfigFile m_config;

};

BOOST_FIXTURE_TEST_SUITE(TestTableConfigSection, TablesConfigSectionFixture)

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

  BOOST_CHECK_EQUAL(m_cs.getLimit(), nCsMaxPackets);
}

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
}

BOOST_AUTO_TEST_CASE(MissingValueCsMaxPackets)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  cs_max_packets\n"
    "}\n";

  const std::string expectedMsg =
    "Invalid value for option \"cs_max_packets\" in \"tables\" section";

  BOOST_CHECK_EXCEPTION(runConfig(CONFIG, true),
                        ConfigFile::Error,
                        bind(&TablesConfigSectionFixture::validateException,
                             this, _1, expectedMsg));
}

BOOST_AUTO_TEST_CASE(InvalidValueCsMaxPackets)
{
  const std::string CONFIG =
    "tables\n"
    "{\n"
    "  cs_max_packets invalid\n"
    "}\n";

  const std::string expectedMsg =
    "Invalid value for option \"cs_max_packets\" in \"tables\" section";

  BOOST_CHECK_EXCEPTION(runConfig(CONFIG, true),
                        ConfigFile::Error,
                        bind(&TablesConfigSectionFixture::validateException,
                             this, _1, expectedMsg));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
