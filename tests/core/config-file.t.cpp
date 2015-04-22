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

#include "core/config-file.hpp"

#include "tests/test-common.hpp"

#include <fstream>
#include <boost/property_tree/info_parser.hpp>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestConfigFile, BaseFixture)

// a
// {
//    akey "avalue"
// }
// b
// {
//   bkey "bvalue"
// }

const std::string CONFIG =
"a\n"
"{\n"
"        akey \"avalue\"\n"
"}\n"
"b\n"
"{\n"
"        bkey \"bvalue\"\n"
"}\n";


// a
// {
//    akey "avalue"
// }
// b
//
//   bkey "bvalue"
// }

const std::string MALFORMED_CONFIG =
"a\n"
"{\n"
"        akey \"avalue\"\n"
"}\n"
"b\n"
"\n"
"        bkey \"bvalue\"\n"
"}\n";

// counts of the respective section counts in config_example.info

const int CONFIG_N_A_SECTIONS = 1;
const int CONFIG_N_B_SECTIONS = 1;

class DummySubscriber
{
public:

  DummySubscriber(ConfigFile& config,
                  int nASections,
                  int nBSections,
                  bool expectDryRun)
    : m_nASections(nASections),
      m_nBSections(nBSections),
      m_nRemainingACallbacks(nASections),
      m_nRemainingBCallbacks(nBSections),
      m_expectDryRun(expectDryRun)
  {

  }

  void
  onA(const ConfigSection& section, bool isDryRun)
  {
    // NFD_LOG_DEBUG("a");
    BOOST_CHECK_EQUAL(isDryRun, m_expectDryRun);
    --m_nRemainingACallbacks;
  }


  void
  onB(const ConfigSection& section, bool isDryRun)
  {
    // NFD_LOG_DEBUG("b");
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

  virtual
  ~DummySubscriber()
  {

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
  DummyAllSubscriber(ConfigFile& config, bool expectDryRun=false)
    : DummySubscriber(config,
                      CONFIG_N_A_SECTIONS,
                      CONFIG_N_B_SECTIONS,
                      expectDryRun)
  {
    config.addSectionHandler("a", bind(&DummySubscriber::onA, this, _1, _2));
    config.addSectionHandler("b", bind(&DummySubscriber::onB, this, _1, _2));
  }

  virtual
  ~DummyAllSubscriber()
  {

  }
};

class DummyOneSubscriber : public DummySubscriber
{
public:
  DummyOneSubscriber(ConfigFile& config,
                     const std::string& sectionName,
                     bool expectDryRun=false)
    : DummySubscriber(config,
                      (sectionName == "a"),
                      (sectionName == "b"),
                      expectDryRun)
  {
    if (sectionName == "a")
      {
        config.addSectionHandler(sectionName, bind(&DummySubscriber::onA, this, _1, _2));
      }
    else if (sectionName == "b")
      {
        config.addSectionHandler(sectionName, bind(&DummySubscriber::onB, this, _1, _2));
      }
    else
      {
        BOOST_FAIL("Test setup error: "
                   << "Unexpected section name "
                   <<"\"" << sectionName << "\"");
      }

  }

  virtual
  ~DummyOneSubscriber()
  {

  }
};

class DummyNoSubscriber : public DummySubscriber
{
public:
  DummyNoSubscriber(ConfigFile& config, bool expectDryRun)
    : DummySubscriber(config, 0, 0, expectDryRun)
  {

  }

  virtual
  ~DummyNoSubscriber()
  {

  }
};

BOOST_AUTO_TEST_CASE(OnConfigStream)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);
  std::ifstream input;

  input.open("tests/core/config_example.info");
  BOOST_REQUIRE(input.is_open());

  file.parse(input, false, "config_example.info");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigStreamEmptyStream)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  std::ifstream input;

  BOOST_CHECK_THROW(file.parse(input, false, "unknown"), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigSection)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  std::istringstream input(CONFIG);
  ConfigSection section;
  boost::property_tree::read_info(input, section);

  file.parse(section, false, "dummy-config");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigString)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  file.parse(CONFIG, false, "dummy-config");

  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigStringEmpty)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  BOOST_CHECK_THROW(file.parse(std::string(), false, "dummy-config"), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigStringMalformed)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  BOOST_CHECK_THROW(file.parse(MALFORMED_CONFIG, false, "dummy-config"), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigStringDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);

  file.parse(CONFIG, true, "dummy-config");

  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigFilename)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  file.parse("tests/core/config_example.info", false);

  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigFilenameNoFile)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  BOOST_CHECK_THROW(file.parse("i_made_this_up.info", false), ConfigFile::Error);

  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigFilenameMalformed)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  BOOST_CHECK_THROW(file.parse("tests/core/config_malformed.info", false), ConfigFile::Error);

  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigStreamDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);
  std::ifstream input;

  input.open("tests/core/config_example.info");
  BOOST_REQUIRE(input.is_open());

  file.parse(input, true, "tests/core/config_example.info");

  BOOST_CHECK(sub.allCallbacksFired());

  input.close();
}

BOOST_AUTO_TEST_CASE(OnConfigFilenameDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);

  file.parse("tests/core/config_example.info", true);
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(OnConfigReplaceSubscriber)
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
  MissingCallbackFixture()
    : m_missingFired(false)
  {
  }

  void
  checkMissingHandler(const std::string& filename,
                      const std::string& sectionName,
                      const ConfigSection& section,
                      bool isDryRun)
  {
    m_missingFired = true;
  }

protected:
  bool m_missingFired;
};



BOOST_FIXTURE_TEST_CASE(OnConfigUncoveredSections, MissingCallbackFixture)
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

BOOST_AUTO_TEST_CASE(OnConfigCoveredByPartialSubscribers)
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
