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

#include "core/config-file.hpp"

#include "tests/test-common.hpp"

#include <boost/property_tree/info_parser.hpp>
#include <fstream>
#include <sstream>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestConfigFile, BaseFixture)

static const std::string CONFIG = R"CONFIG(
  a
  {
    akey avalue
  }
  b
  {
    bkey bvalue
  }
)CONFIG";

// counts of the respective section counts in config_example.info
const int CONFIG_N_A_SECTIONS = 1;
const int CONFIG_N_B_SECTIONS = 1;

class DummySubscriber
{
public:
  DummySubscriber(ConfigFile& config, int nASections, int nBSections, bool expectDryRun)
    : m_nASections(nASections)
    , m_nBSections(nBSections)
    , m_nRemainingACallbacks(nASections)
    , m_nRemainingBCallbacks(nBSections)
    , m_expectDryRun(expectDryRun)
  {
  }

  void
  onA(const ConfigSection& section, bool isDryRun)
  {
    BOOST_CHECK_EQUAL(isDryRun, m_expectDryRun);
    --m_nRemainingACallbacks;
  }

  void
  onB(const ConfigSection& section, bool isDryRun)
  {
    BOOST_CHECK_EQUAL(isDryRun, m_expectDryRun);
    --m_nRemainingBCallbacks;
  }

  bool
  allCallbacksFired() const
  {
    return m_nRemainingACallbacks == 0 &&
      m_nRemainingBCallbacks == 0;
  }

  bool
  noCallbacksFired() const
  {
    return m_nRemainingACallbacks == m_nASections &&
      m_nRemainingBCallbacks == m_nBSections;
  }

private:
  int m_nASections;
  int m_nBSections;
  int m_nRemainingACallbacks;
  int m_nRemainingBCallbacks;
  bool m_expectDryRun;
};

class DummyAllSubscriber : public DummySubscriber
{
public:
  explicit
  DummyAllSubscriber(ConfigFile& config, bool expectDryRun = false)
    : DummySubscriber(config, CONFIG_N_A_SECTIONS, CONFIG_N_B_SECTIONS, expectDryRun)
  {
    config.addSectionHandler("a", bind(&DummySubscriber::onA, this, _1, _2));
    config.addSectionHandler("b", bind(&DummySubscriber::onB, this, _1, _2));
  }
};

class DummyOneSubscriber : public DummySubscriber
{
public:
  DummyOneSubscriber(ConfigFile& config, const std::string& sectionName, bool expectDryRun = false)
    : DummySubscriber(config,
                      (sectionName == "a"),
                      (sectionName == "b"),
                      expectDryRun)
  {
    if (sectionName == "a") {
      config.addSectionHandler(sectionName, bind(&DummySubscriber::onA, this, _1, _2));
    }
    else if (sectionName == "b") {
      config.addSectionHandler(sectionName, bind(&DummySubscriber::onB, this, _1, _2));
    }
    else {
      BOOST_FAIL("Test setup error: Unexpected section name '" << sectionName << "'");
    }
  }
};

class DummyNoSubscriber : public DummySubscriber
{
public:
  DummyNoSubscriber(ConfigFile& config, bool expectDryRun)
    : DummySubscriber(config, 0, 0, expectDryRun)
  {
  }
};

BOOST_AUTO_TEST_CASE(ParseFromStream)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  std::ifstream input("tests/core/config_example.info");
  BOOST_REQUIRE(input.is_open());

  file.parse(input, false, "config_example.info");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromEmptyStream)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  std::istringstream input;

  file.parse(input, false, "empty");
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromStreamDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);

  std::ifstream input("tests/core/config_example.info");
  BOOST_REQUIRE(input.is_open());

  file.parse(input, true, "tests/core/config_example.info");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromConfigSection)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  std::istringstream input(CONFIG);
  ConfigSection section;
  boost::property_tree::read_info(input, section);

  file.parse(section, false, "dummy-config");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromString)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  file.parse(CONFIG, false, "dummy-config");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromEmptyString)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  file.parse("", false, "empty");
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromMalformedString)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  const std::string malformed = R"CONFIG(
    a
    {
      akey avalue
    }
    b
      bkey bvalue
    }
  )CONFIG";

  BOOST_CHECK_THROW(file.parse(malformed, false, "dummy-config"), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromStringDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);

  file.parse(CONFIG, true, "dummy-config");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromFilename)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  file.parse("tests/core/config_example.info", false);
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromFilenameNonExistent)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  BOOST_CHECK_THROW(file.parse("i_made_this_up.info", false), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromFilenameMalformed)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  BOOST_CHECK_THROW(file.parse("tests/core/config_malformed.info", false), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromFilenameDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);

  file.parse("tests/core/config_example.info", true);
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ReplaceSubscriber)
{
  ConfigFile file;
  DummyAllSubscriber sub1(file);
  DummyAllSubscriber sub2(file);

  file.parse(CONFIG, false, "dummy-config");

  BOOST_CHECK(sub1.noCallbacksFired());
  BOOST_CHECK(sub2.allCallbacksFired());
}

class MissingCallbackFixture : public BaseFixture
{
public:
  void
  checkMissingHandler(const std::string& filename,
                      const std::string& sectionName,
                      const ConfigSection& section,
                      bool isDryRun)
  {
    m_missingFired = true;
  }

protected:
  bool m_missingFired = false;
};

BOOST_FIXTURE_TEST_CASE(UncoveredSections, MissingCallbackFixture)
{
  ConfigFile file;
  BOOST_REQUIRE_THROW(file.parse(CONFIG, false, "dummy-config"), ConfigFile::Error);

  ConfigFile permissiveFile(bind(&MissingCallbackFixture::checkMissingHandler,
                                 this, _1, _2, _3, _4));
  DummyOneSubscriber subA(permissiveFile, "a");

  BOOST_REQUIRE_NO_THROW(permissiveFile.parse(CONFIG, false, "dummy-config"));
  BOOST_CHECK(subA.allCallbacksFired());
  BOOST_CHECK(m_missingFired);
}

BOOST_AUTO_TEST_CASE(CoveredByPartialSubscribers)
{
  ConfigFile file;
  DummyOneSubscriber subA(file, "a");
  DummyOneSubscriber subB(file, "b");

  file.parse(CONFIG, false, "dummy-config");

  BOOST_CHECK(subA.allCallbacksFired());
  BOOST_CHECK(subB.allCallbacksFired());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
