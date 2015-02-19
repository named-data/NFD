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

#include "core/logger.hpp"

#include "tests/test-common.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestLogger, BaseFixture)

class LoggerFixture : protected BaseFixture
{
public:
  LoggerFixture()
    : m_savedBuf(std::clog.rdbuf())
    , m_savedLevel(LoggerFactory::getInstance().getDefaultLevel())
  {
    std::clog.rdbuf(m_buffer.rdbuf());
  }

  ~LoggerFixture()
  {
    std::clog.rdbuf(m_savedBuf);
    LoggerFactory::getInstance().setDefaultLevel(m_savedLevel);
  }

  std::stringstream m_buffer;
  std::streambuf* m_savedBuf;
  LogLevel m_savedLevel;

};

BOOST_FIXTURE_TEST_CASE(Basic, LoggerFixture)
{
  using namespace ndn::time;
  using std::string;

  const ndn::time::microseconds::rep ONE_SECOND = 1000000;

  NFD_LOG_INIT("BasicTests");
  g_logger.setLogLevel(LOG_ALL);

  const string EXPECTED[] =
    {
      "TRACE:",   "[BasicTests]", "trace-message-JHGFDSR^1\n",
      "DEBUG:",   "[BasicTests]", "debug-message-IGg2474fdksd-fo-151617\n",
      "WARNING:", "[BasicTests]", "warning-message-XXXhdhd111x\n",
      "INFO:",    "[BasicTests]", "info-message-Jjxjshj13\n",
      "ERROR:",   "[BasicTests]", "error-message-!#$&^%$#@\n",
      "FATAL:",   "[BasicTests]", "fatal-message-JJSjaamcng\n",
    };

  const size_t N_EXPECTED = sizeof(EXPECTED) / sizeof(string);

  microseconds::rep before =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  NFD_LOG_TRACE("trace-message-JHGFDSR^1");
  NFD_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
  NFD_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
  NFD_LOG_INFO("info-message-Jjxjshj13");
  NFD_LOG_ERROR("error-message-!#$&^%$#@");
  NFD_LOG_FATAL("fatal-message-JJSjaamcng");

  microseconds::rep after =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  const string buffer = m_buffer.str();

  std::vector<string> components;
  boost::split(components, buffer, boost::is_any_of(" ,\n"));

  // std::cout << components.size() << " for " << moduleName  << std::endl;
  // for (size_t i = 0; i < components.size(); ++i)
  //   {
  //     std::cout << "-> " << components[i] << std::endl;
  //   }

  // expected + number of timestamps (one per log statement) + trailing newline of last statement
  BOOST_REQUIRE_EQUAL(components.size(), N_EXPECTED + 6 + 1);

  std::vector<std::string>::const_iterator componentIter = components.begin();
  for (size_t i = 0; i < N_EXPECTED; ++i)
    {
      // timestamp LOG_LEVEL: [ModuleName] message\n

      const string& timestamp = *componentIter;
      // std::cout << "timestamp = " << timestamp << std::endl;
      ++componentIter;

      size_t timeDelimiterPosition = timestamp.find(".");

      BOOST_REQUIRE_NE(string::npos, timeDelimiterPosition);

      string secondsString = timestamp.substr(0, timeDelimiterPosition);
      string usecondsString = timestamp.substr(timeDelimiterPosition + 1);

      microseconds::rep extractedTime =
        ONE_SECOND * boost::lexical_cast<microseconds::rep>(secondsString) +
        boost::lexical_cast<microseconds::rep>(usecondsString);

      // std::cout << "before=" << before
      //           << " extracted=" << extractedTime
      //           << " after=" << after << std::endl;


      BOOST_CHECK_LE(before, extractedTime);
      BOOST_CHECK_LE(extractedTime, after);

      // LOG_LEVEL:
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      // [ModuleName]
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      const string& message = *componentIter;

      // std::cout << "message = " << message << std::endl;

      // add back the newline that we split on
      BOOST_CHECK_EQUAL(message + "\n", EXPECTED[i]);
      ++componentIter;
    }

}


BOOST_FIXTURE_TEST_CASE(ConfigureFactory, LoggerFixture)
{
  using namespace ndn::time;
  using std::string;

  const ndn::time::microseconds::rep ONE_SECOND = 1000000;

  NFD_LOG_INIT("ConfigureFactoryTests");

  const string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level INFO\n"
    "}\n";

  LoggerFactory::getInstance().setDefaultLevel(LOG_ALL);

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

  BOOST_REQUIRE_EQUAL(LoggerFactory::getInstance().getDefaultLevel(), LOG_INFO);

  const std::string EXPECTED[] =
    {
      "WARNING:", "[ConfigureFactoryTests]", "warning-message-XXXhdhd111x\n",
      "INFO:",    "[ConfigureFactoryTests]", "info-message-Jjxjshj13\n",
      "ERROR:",   "[ConfigureFactoryTests]", "error-message-!#$&^%$#@\n",
      "FATAL:",   "[ConfigureFactoryTests]", "fatal-message-JJSjaamcng\n",
    };

  const size_t N_EXPECTED = sizeof(EXPECTED) / sizeof(std::string);

  microseconds::rep before =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  NFD_LOG_TRACE("trace-message-JHGFDSR^1");
  NFD_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
  NFD_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
  NFD_LOG_INFO("info-message-Jjxjshj13");
  NFD_LOG_ERROR("error-message-!#$&^%$#@");
  NFD_LOG_FATAL("fatal-message-JJSjaamcng");

  microseconds::rep after =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  const string buffer = m_buffer.str();

  std::vector<string> components;
  boost::split(components, buffer, boost::is_any_of(" ,\n"));

  // std::cout << components.size() << " for " << moduleName  << std::endl;
  // for (size_t i = 0; i < components.size(); ++i)
  //   {
  //     std::cout << "-> " << components[i] << std::endl;
  //   }

  // expected + number of timestamps (one per log statement) + trailing newline of last statement
  BOOST_REQUIRE_EQUAL(components.size(), N_EXPECTED + 4 + 1);

  std::vector<std::string>::const_iterator componentIter = components.begin();
  for (size_t i = 0; i < N_EXPECTED; ++i)
    {
      // timestamp LOG_LEVEL: [ModuleName] message\n

      const string& timestamp = *componentIter;
      // std::cout << "timestamp = " << timestamp << std::endl;
      ++componentIter;

      size_t timeDelimiterPosition = timestamp.find(".");

      BOOST_REQUIRE_NE(string::npos, timeDelimiterPosition);

      string secondsString = timestamp.substr(0, timeDelimiterPosition);
      string usecondsString = timestamp.substr(timeDelimiterPosition + 1);

      microseconds::rep extractedTime =
        ONE_SECOND * boost::lexical_cast<microseconds::rep>(secondsString) +
        boost::lexical_cast<microseconds::rep>(usecondsString);

      // std::cout << "before=" << before
      //           << " extracted=" << extractedTime
      //           << " after=" << after << std::endl;

      BOOST_CHECK_LE(before, extractedTime);
      BOOST_CHECK_LE(extractedTime, after);

      // LOG_LEVEL:
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      // [ModuleName]
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      const string& message = *componentIter;

      // std::cout << "message = " << message << std::endl;

      // add back the newline that we split on
      BOOST_CHECK_EQUAL(message + "\n", EXPECTED[i]);
      ++componentIter;
    }
}

BOOST_FIXTURE_TEST_CASE(TestNumberLevel, LoggerFixture)
{
  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level 2\n" // equivalent of WARN
    "}\n";

  LoggerFactory::getInstance().setDefaultLevel(LOG_ALL);

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

  BOOST_REQUIRE_EQUAL(LoggerFactory::getInstance().getDefaultLevel(), LOG_WARN);
}

static void
testModuleBPrint()
{
  NFD_LOG_INIT("TestModuleB");
  NFD_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
}

BOOST_FIXTURE_TEST_CASE(LimitModules, LoggerFixture)
{
  using namespace ndn::time;
  using std::string;

  NFD_LOG_INIT("TestModuleA");

  const ndn::time::microseconds::rep ONE_SECOND = 1000000;

  const std::string EXPECTED[] =
    {
      "WARNING:", "[TestModuleA]", "warning-message-XXXhdhd111x\n",
    };

  const size_t N_EXPECTED = sizeof(EXPECTED) / sizeof(std::string);

  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level WARN\n"
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

  microseconds::rep before =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  // this should print
  NFD_LOG_WARN("warning-message-XXXhdhd11" << 1 << "x");

  // this should not because it's level is < WARN
  testModuleBPrint();

  microseconds::rep after =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  const string buffer = m_buffer.str();

  std::vector<string> components;
  boost::split(components, buffer, boost::is_any_of(" ,\n"));

  // expected + number of timestamps (one per log statement) + trailing newline of last statement
  BOOST_REQUIRE_EQUAL(components.size(), N_EXPECTED + 1 + 1);

  std::vector<std::string>::const_iterator componentIter = components.begin();
  for (size_t i = 0; i < N_EXPECTED; ++i)
    {
      // timestamp LOG_LEVEL: [ModuleName] message\n

      const string& timestamp = *componentIter;
      // std::cout << "timestamp = " << timestamp << std::endl;
      ++componentIter;

      size_t timeDelimiterPosition = timestamp.find(".");

      BOOST_REQUIRE_NE(string::npos, timeDelimiterPosition);

      string secondsString = timestamp.substr(0, timeDelimiterPosition);
      string usecondsString = timestamp.substr(timeDelimiterPosition + 1);

      microseconds::rep extractedTime =
        ONE_SECOND * boost::lexical_cast<microseconds::rep>(secondsString) +
        boost::lexical_cast<microseconds::rep>(usecondsString);

      // std::cout << "before=" << before
      //           << " extracted=" << extractedTime
      //           << " after=" << after << std::endl;

      BOOST_CHECK_LE(before, extractedTime);
      BOOST_CHECK_LE(extractedTime, after);

      // LOG_LEVEL:
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      // [ModuleName]
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      const string& message = *componentIter;

      // std::cout << "message = " << message << std::endl;

      // add back the newline that we split on
      BOOST_CHECK_EQUAL(message + "\n", EXPECTED[i]);
      ++componentIter;
    }
}

BOOST_FIXTURE_TEST_CASE(ExplicitlySetModule, LoggerFixture)
{
  using namespace ndn::time;
  using std::string;

  NFD_LOG_INIT("TestModuleA");

  const ndn::time::microseconds::rep ONE_SECOND = 1000000;

  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level WARN\n"
    "  TestModuleB DEBUG\n"
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

  microseconds::rep before =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

 // this should print
  NFD_LOG_WARN("warning-message-XXXhdhd11" << 1 << "x");

  // this too because its level is explicitly set to DEBUG
  testModuleBPrint();

  microseconds::rep after =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  const std::string EXPECTED[] =
    {
      "WARNING:", "[TestModuleA]", "warning-message-XXXhdhd111x\n",
      "DEBUG:",   "[TestModuleB]", "debug-message-IGg2474fdksd-fo-151617\n",
    };

  const size_t N_EXPECTED = sizeof(EXPECTED) / sizeof(std::string);

  const string buffer = m_buffer.str();

  std::vector<string> components;
  boost::split(components, buffer, boost::is_any_of(" ,\n"));

  // for (size_t i = 0; i < components.size(); ++i)
  //   {
  //     std::cout << "-> " << components[i] << std::endl;
  //   }

  // expected + number of timestamps (one per log statement) + trailing newline of last statement
  BOOST_REQUIRE_EQUAL(components.size(), N_EXPECTED + 2 + 1);

  std::vector<std::string>::const_iterator componentIter = components.begin();
  for (size_t i = 0; i < N_EXPECTED; ++i)
    {
      // timestamp LOG_LEVEL: [ModuleName] message\n

      const string& timestamp = *componentIter;
      // std::cout << "timestamp = " << timestamp << std::endl;
      ++componentIter;

      size_t timeDelimiterPosition = timestamp.find(".");

      BOOST_REQUIRE_NE(string::npos, timeDelimiterPosition);

      string secondsString = timestamp.substr(0, timeDelimiterPosition);
      string usecondsString = timestamp.substr(timeDelimiterPosition + 1);

      microseconds::rep extractedTime =
        ONE_SECOND * boost::lexical_cast<microseconds::rep>(secondsString) +
        boost::lexical_cast<microseconds::rep>(usecondsString);

      // std::cout << "before=" << before
      //           << " extracted=" << extractedTime
      //           << " after=" << after << std::endl;

      BOOST_CHECK_LE(before, extractedTime);
      BOOST_CHECK_LE(extractedTime, after);

      // LOG_LEVEL:
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      // [ModuleName]
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      const string& message = *componentIter;

      // std::cout << "message = " << message << std::endl;

      // add back the newline that we split on
      BOOST_CHECK_EQUAL(message + "\n", EXPECTED[i]);
      ++componentIter;
    }
}

BOOST_FIXTURE_TEST_CASE(UnknownModule, LoggerFixture)
{
  using namespace ndn::time;
  using std::string;

  const ndn::time::microseconds::rep ONE_SECOND = 1000000;

  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level DEBUG\n"
    "  TestMadeUpModule INFO\n"
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setDefaultLevel(LOG_ALL);
  LoggerFactory::getInstance().setConfigFile(config);

  const std::string EXPECTED = "DEBUG: [LoggerFactory] "
    "Failed to configure logging level for module \"TestMadeUpModule\" (module not found)\n";

  microseconds::rep before =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

  microseconds::rep after =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  const string buffer = m_buffer.str();

  const size_t firstSpace = buffer.find(" ");
  BOOST_REQUIRE(firstSpace != string::npos);

  const string timestamp = buffer.substr(0, firstSpace);
  const string message = buffer.substr(firstSpace + 1);

  size_t timeDelimiterPosition = timestamp.find(".");

  BOOST_REQUIRE_NE(string::npos, timeDelimiterPosition);

  string secondsString = timestamp.substr(0, timeDelimiterPosition);
  string usecondsString = timestamp.substr(timeDelimiterPosition + 1);

  microseconds::rep extractedTime =
    ONE_SECOND * boost::lexical_cast<microseconds::rep>(secondsString) +
    boost::lexical_cast<microseconds::rep>(usecondsString);

  // std::cout << "before=" << before
  //           << " extracted=" << extractedTime
  //           << " after=" << after << std::endl;

  BOOST_CHECK_LE(before, extractedTime);
  BOOST_CHECK_LE(extractedTime, after);

  BOOST_CHECK_EQUAL(message, EXPECTED);
}

static bool
checkError(const LoggerFactory::Error& error, const std::string& expected)
{
  return error.what() == expected;
}

BOOST_FIXTURE_TEST_CASE(UnknownLevelString, LoggerFixture)
{
  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level TestMadeUpLevel\n"
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  BOOST_REQUIRE_EXCEPTION(config.parse(LOG_CONFIG, false, "LOG_CONFIG"),
                          LoggerFactory::Error,
                          bind(&checkError,
                               _1,
                               "Unsupported logging level \"TestMadeUpLevel\""));
}

class InClassLogger : public LoggerFixture
{
public:

  InClassLogger()
  {
    g_logger.setLogLevel(LOG_ALL);
  }

  void
  writeLogs()
  {
    NFD_LOG_TRACE("trace-message-JHGFDSR^1");
    NFD_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
    NFD_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
    NFD_LOG_INFO("info-message-Jjxjshj13");
    NFD_LOG_ERROR("error-message-!#$&^%$#@");
    NFD_LOG_FATAL("fatal-message-JJSjaamcng");
  }

private:
  NFD_LOG_INCLASS_DECLARE();
};

NFD_LOG_INCLASS_DEFINE(InClassLogger, "InClassLogger");

BOOST_FIXTURE_TEST_CASE(InClass, InClassLogger)
{
  using namespace ndn::time;
  using std::string;

  const ndn::time::microseconds::rep ONE_SECOND = 1000000;

  microseconds::rep before =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  writeLogs();

  microseconds::rep after =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  const string EXPECTED[] =
    {
      "TRACE:",   "[InClassLogger]", "trace-message-JHGFDSR^1\n",
      "DEBUG:",   "[InClassLogger]", "debug-message-IGg2474fdksd-fo-151617\n",
      "WARNING:", "[InClassLogger]", "warning-message-XXXhdhd111x\n",
      "INFO:",    "[InClassLogger]", "info-message-Jjxjshj13\n",
      "ERROR:",   "[InClassLogger]", "error-message-!#$&^%$#@\n",
      "FATAL:",   "[InClassLogger]", "fatal-message-JJSjaamcng\n",
    };

  const size_t N_EXPECTED = sizeof(EXPECTED) / sizeof(string);

  const string buffer = m_buffer.str();

  std::vector<string> components;
  boost::split(components, buffer, boost::is_any_of(" ,\n"));

  // expected + number of timestamps (one per log statement) + trailing newline of last statement
  BOOST_REQUIRE_EQUAL(components.size(), N_EXPECTED + 6 + 1);

  std::vector<std::string>::const_iterator componentIter = components.begin();
  for (size_t i = 0; i < N_EXPECTED; ++i)
    {
      // timestamp LOG_LEVEL: [ModuleName] message\n

      const string& timestamp = *componentIter;
      // std::cout << "timestamp = " << timestamp << std::endl;
      ++componentIter;

      size_t timeDelimiterPosition = timestamp.find(".");

      BOOST_REQUIRE_NE(string::npos, timeDelimiterPosition);

      string secondsString = timestamp.substr(0, timeDelimiterPosition);
      string usecondsString = timestamp.substr(timeDelimiterPosition + 1);

      microseconds::rep extractedTime =
        ONE_SECOND * boost::lexical_cast<microseconds::rep>(secondsString) +
        boost::lexical_cast<microseconds::rep>(usecondsString);

      // std::cout << "before=" << before
      //           << " extracted=" << extractedTime
      //           << " after=" << after << std::endl;

      BOOST_CHECK_LE(before, extractedTime);
      BOOST_CHECK_LE(extractedTime, after);

      // LOG_LEVEL:
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      // [ModuleName]
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      const string& message = *componentIter;

      // std::cout << "message = " << message << std::endl;

      // add back the newline that we split on
      BOOST_CHECK_EQUAL(message + "\n", EXPECTED[i]);
      ++componentIter;
    }
}


template<class T>
class InClassTemplateLogger : public LoggerFixture
{
public:
  InClassTemplateLogger()
  {
    g_logger.setLogLevel(LOG_ALL);
  }

  void
  writeLogs()
  {
    NFD_LOG_TRACE("trace-message-JHGFDSR^1");
    NFD_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
    NFD_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
    NFD_LOG_INFO("info-message-Jjxjshj13");
    NFD_LOG_ERROR("error-message-!#$&^%$#@");
    NFD_LOG_FATAL("fatal-message-JJSjaamcng");
  }

private:
  NFD_LOG_INCLASS_DECLARE();
};

NFD_LOG_INCLASS_TEMPLATE_DEFINE(InClassTemplateLogger, "GenericInClassTemplateLogger");
NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(InClassTemplateLogger, int, "IntInClassLogger");

BOOST_FIXTURE_TEST_CASE(GenericInTemplatedClass, InClassTemplateLogger<bool>)
{
  using namespace ndn::time;
  using std::string;

  const ndn::time::microseconds::rep ONE_SECOND = 1000000;

  microseconds::rep before =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  writeLogs();

  microseconds::rep after =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  const string EXPECTED[] =
    {
      "TRACE:",   "[GenericInClassTemplateLogger]", "trace-message-JHGFDSR^1\n",
      "DEBUG:",   "[GenericInClassTemplateLogger]", "debug-message-IGg2474fdksd-fo-151617\n",
      "WARNING:", "[GenericInClassTemplateLogger]", "warning-message-XXXhdhd111x\n",
      "INFO:",    "[GenericInClassTemplateLogger]", "info-message-Jjxjshj13\n",
      "ERROR:",   "[GenericInClassTemplateLogger]", "error-message-!#$&^%$#@\n",
      "FATAL:",   "[GenericInClassTemplateLogger]", "fatal-message-JJSjaamcng\n",
    };

  const size_t N_EXPECTED = sizeof(EXPECTED) / sizeof(string);

  const string buffer = m_buffer.str();

  std::vector<string> components;
  boost::split(components, buffer, boost::is_any_of(" ,\n"));

  // expected + number of timestamps (one per log statement) + trailing newline of last statement
  BOOST_REQUIRE_EQUAL(components.size(), N_EXPECTED + 6 + 1);

  std::vector<std::string>::const_iterator componentIter = components.begin();
  for (size_t i = 0; i < N_EXPECTED; ++i)
    {
      // timestamp LOG_LEVEL: [ModuleName] message\n

      const string& timestamp = *componentIter;
      // std::cout << "timestamp = " << timestamp << std::endl;
      ++componentIter;

      size_t timeDelimiterPosition = timestamp.find(".");

      BOOST_REQUIRE_NE(string::npos, timeDelimiterPosition);

      string secondsString = timestamp.substr(0, timeDelimiterPosition);
      string usecondsString = timestamp.substr(timeDelimiterPosition + 1);

      microseconds::rep extractedTime =
        ONE_SECOND * boost::lexical_cast<microseconds::rep>(secondsString) +
        boost::lexical_cast<microseconds::rep>(usecondsString);

      // std::cout << "before=" << before
      //           << " extracted=" << extractedTime
      //           << " after=" << after << std::endl;

      BOOST_CHECK_LE(before, extractedTime);
      BOOST_CHECK_LE(extractedTime, after);

      // LOG_LEVEL:
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      // [ModuleName]
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      const string& message = *componentIter;

      // std::cout << "message = " << message << std::endl;

      // add back the newline that we split on
      BOOST_CHECK_EQUAL(message + "\n", EXPECTED[i]);
      ++componentIter;
    }
}


BOOST_FIXTURE_TEST_CASE(SpecializedInTemplatedClass, InClassTemplateLogger<int>)
{
  using namespace ndn::time;
  using std::string;

  const ndn::time::microseconds::rep ONE_SECOND = 1000000;

  microseconds::rep before =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  writeLogs();

  microseconds::rep after =
    duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  const string EXPECTED[] =
    {
      "TRACE:",   "[IntInClassLogger]", "trace-message-JHGFDSR^1\n",
      "DEBUG:",   "[IntInClassLogger]", "debug-message-IGg2474fdksd-fo-151617\n",
      "WARNING:", "[IntInClassLogger]", "warning-message-XXXhdhd111x\n",
      "INFO:",    "[IntInClassLogger]", "info-message-Jjxjshj13\n",
      "ERROR:",   "[IntInClassLogger]", "error-message-!#$&^%$#@\n",
      "FATAL:",   "[IntInClassLogger]", "fatal-message-JJSjaamcng\n",
    };

  const size_t N_EXPECTED = sizeof(EXPECTED) / sizeof(string);

  const string buffer = m_buffer.str();

  std::vector<string> components;
  boost::split(components, buffer, boost::is_any_of(" ,\n"));

  // expected + number of timestamps (one per log statement) + trailing newline of last statement
  BOOST_REQUIRE_EQUAL(components.size(), N_EXPECTED + 6 + 1);

  std::vector<std::string>::const_iterator componentIter = components.begin();
  for (size_t i = 0; i < N_EXPECTED; ++i)
    {
      // timestamp LOG_LEVEL: [ModuleName] message\n

      const string& timestamp = *componentIter;
      // std::cout << "timestamp = " << timestamp << std::endl;
      ++componentIter;

      size_t timeDelimiterPosition = timestamp.find(".");

      BOOST_REQUIRE_NE(string::npos, timeDelimiterPosition);

      string secondsString = timestamp.substr(0, timeDelimiterPosition);
      string usecondsString = timestamp.substr(timeDelimiterPosition + 1);

      microseconds::rep extractedTime =
        ONE_SECOND * boost::lexical_cast<microseconds::rep>(secondsString) +
        boost::lexical_cast<microseconds::rep>(usecondsString);

      // std::cout << "before=" << before << " extracted=" << extractedTime << " after=" << after << std::endl;

      BOOST_CHECK_LE(before, extractedTime);
      BOOST_CHECK_LE(extractedTime, after);

      // LOG_LEVEL:
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      // [ModuleName]
      BOOST_CHECK_EQUAL(*componentIter, EXPECTED[i]);
      ++componentIter;
      ++i;

      const string& message = *componentIter;

      // std::cout << "message = " << message << std::endl;

      // add back the newline that we split on
      BOOST_CHECK_EQUAL(message + "\n", EXPECTED[i]);
      ++componentIter;
    }
}

BOOST_FIXTURE_TEST_CASE(LoggerFactoryListModules, LoggerFixture)
{
  std::set<std::string> testCaseLoggers{"LoggerFactoryListModules1", "LoggerFactoryListModules2"};

  for (const auto& name : testCaseLoggers) {
    LoggerFactory::create(name);
  }

  auto&& modules = LoggerFactory::getInstance().getModules();
  BOOST_CHECK_GE(modules.size(), 2);

  for (const auto& name : modules) {
    testCaseLoggers.erase(name);
  }

  BOOST_CHECK_EQUAL(testCaseLoggers.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
