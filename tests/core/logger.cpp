/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#include "core/logger.hpp"

#include <boost/test/unit_test.hpp>

#include <iostream>

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreLogger, BaseFixture)

class LoggerFixture : protected BaseFixture
{
public:
  LoggerFixture()
    : m_savedBuf(std::cerr.rdbuf())
    , m_savedLevel(LoggerFactory::getInstance().getDefaultLevel())
  {
    std::cerr.rdbuf(m_buffer.rdbuf());
  }

  ~LoggerFixture()
  {
    std::cerr.rdbuf(m_savedBuf);
    LoggerFactory::getInstance().setDefaultLevel(m_savedLevel);
  }

  std::stringstream m_buffer;
  std::streambuf* m_savedBuf;
  LogLevel m_savedLevel;
};

BOOST_FIXTURE_TEST_CASE(Basic, LoggerFixture)
{
  NFD_LOG_INIT("BasicTests");
  g_logger.setLogLevel(LOG_ALL);

  NFD_LOG_TRACE("trace message JHGFDSR^1");
  NFD_LOG_DEBUG("debug message IGg2474fdksd fo " << 15 << 16 << 17);
  NFD_LOG_WARN("warning message XXXhdhd11" << 1 << "x");
  NFD_LOG_INFO("info message Jjxjshj13");
  NFD_LOG_ERROR("error message !#$&^%$#@");
  NFD_LOG_FATAL("fatal message JJSjaamcng");

  BOOST_CHECK_EQUAL(m_buffer.str(),
                    "TRACE: [BasicTests] trace message JHGFDSR^1\n"
                    "DEBUG: [BasicTests] debug message IGg2474fdksd fo 151617\n"
                    "WARNING: [BasicTests] warning message XXXhdhd111x\n"
                    "INFO: [BasicTests] info message Jjxjshj13\n"
                    "ERROR: [BasicTests] error message !#$&^%$#@\n"
                    "FATAL: [BasicTests] fatal message JJSjaamcng\n"
                    );
}

BOOST_FIXTURE_TEST_CASE(ConfigureFactory, LoggerFixture)
{
  NFD_LOG_INIT("ConfigureFactoryTests");

  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level INFO\n"
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

  NFD_LOG_TRACE("trace message JHGFDSR^1");
  NFD_LOG_DEBUG("debug message IGg2474fdksd fo " << 15 << 16 << 17);
  NFD_LOG_WARN("warning message XXXhdhd11" << 1 << "x");
  NFD_LOG_INFO("info message Jjxjshj13");
  NFD_LOG_ERROR("error message !#$&^%$#@");
  NFD_LOG_FATAL("fatal message JJSjaamcng");

  BOOST_CHECK_EQUAL(m_buffer.str(),
                    "WARNING: [ConfigureFactoryTests] warning message XXXhdhd111x\n"
                    "INFO: [ConfigureFactoryTests] info message Jjxjshj13\n"
                    "ERROR: [ConfigureFactoryTests] error message !#$&^%$#@\n"
                    "FATAL: [ConfigureFactoryTests] fatal message JJSjaamcng\n"
                    );
}

BOOST_FIXTURE_TEST_CASE(TestNumberLevel, LoggerFixture)
{
  NFD_LOG_INIT("TestNumberLevel");

  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level 2\n" // equivalent of WARN
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

  NFD_LOG_TRACE("trace message JHGFDSR^1");
  NFD_LOG_DEBUG("debug message IGg2474fdksd fo " << 15 << 16 << 17);
  NFD_LOG_WARN("warning message XXXhdhd11" << 1 << "x");
  NFD_LOG_INFO("info message Jjxjshj13");
  NFD_LOG_ERROR("error message !#$&^%$#@");
  NFD_LOG_FATAL("fatal message JJSjaamcng");

  BOOST_CHECK_EQUAL(m_buffer.str(),
                    "WARNING: [TestNumberLevel] warning message XXXhdhd111x\n"
                    "ERROR: [TestNumberLevel] error message !#$&^%$#@\n"
                    "FATAL: [TestNumberLevel] fatal message JJSjaamcng\n"
                    );
}

static void
testModuleBPrint()
{
  NFD_LOG_INIT("TestModuleB");
  NFD_LOG_DEBUG("debug message IGg2474fdksd fo " << 15 << 16 << 17);
}

BOOST_FIXTURE_TEST_CASE(LimitModules, LoggerFixture)
{
  NFD_LOG_INIT("TestModuleA");

  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level WARN\n"
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

 // this should print
  NFD_LOG_WARN("warning message XXXhdhd11" << 1 << "x");

  // this should not because it's level is < WARN
  testModuleBPrint();

  BOOST_CHECK_EQUAL(m_buffer.str(),
                    "WARNING: [TestModuleA] warning message XXXhdhd111x\n"
                    );
}

BOOST_FIXTURE_TEST_CASE(ExplicitlySetModule, LoggerFixture)
{
  NFD_LOG_INIT("TestModuleA");

  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  default_level WARN\n"
    "  TestModuleB DEBUG\n"
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  config.parse(LOG_CONFIG, false, "LOG_CONFIG");

 // this should print
  NFD_LOG_WARN("warning message XXXhdhd11" << 1 << "x");

  // this too because its level is explicitly set to DEBUG
  testModuleBPrint();

  BOOST_CHECK_EQUAL(m_buffer.str(),
                    "WARNING: [TestModuleA] warning message XXXhdhd111x\n"
                    "DEBUG: [TestModuleB] debug message IGg2474fdksd fo 151617\n"
                    );
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

BOOST_FIXTURE_TEST_CASE(UnknownOption, LoggerFixture)
{
  const std::string LOG_CONFIG =
    "log\n"
    "{\n"
    "  TestMadeUpOption\n"
    "}\n";

  ConfigFile config;
  LoggerFactory::getInstance().setConfigFile(config);

  BOOST_REQUIRE_EXCEPTION(config.parse(LOG_CONFIG, false, "LOG_CONFIG"),
                          LoggerFactory::Error,
                          bind(&checkError,
                               _1,
                               "No logging level found for option \"TestMadeUpOption\""));
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
    NFD_LOG_TRACE("trace message JHGFDSR^1");
    NFD_LOG_DEBUG("debug message IGg2474fdksd fo " << 15 << 16 << 17);
    NFD_LOG_WARN("warning message XXXhdhd11" << 1 << "x");
    NFD_LOG_INFO("info message Jjxjshj13");
    NFD_LOG_ERROR("error message !#$&^%$#@");
    NFD_LOG_FATAL("fatal message JJSjaamcng");
  }

private:
  NFD_LOG_INCLASS_DECLARE();
};

NFD_LOG_INCLASS_DEFINE(InClassLogger, "InClassLogger");

BOOST_FIXTURE_TEST_CASE(InClass, InClassLogger)
{
  writeLogs();

  BOOST_CHECK_EQUAL(m_buffer.str(),
                    "TRACE: [InClassLogger] trace message JHGFDSR^1\n"
                    "DEBUG: [InClassLogger] debug message IGg2474fdksd fo 151617\n"
                    "WARNING: [InClassLogger] warning message XXXhdhd111x\n"
                    "INFO: [InClassLogger] info message Jjxjshj13\n"
                    "ERROR: [InClassLogger] error message !#$&^%$#@\n"
                    "FATAL: [InClassLogger] fatal message JJSjaamcng\n"
                    );
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
    NFD_LOG_TRACE("trace message JHGFDSR^1");
    NFD_LOG_DEBUG("debug message IGg2474fdksd fo " << 15 << 16 << 17);
    NFD_LOG_WARN("warning message XXXhdhd11" << 1 << "x");
    NFD_LOG_INFO("info message Jjxjshj13");
    NFD_LOG_ERROR("error message !#$&^%$#@");
    NFD_LOG_FATAL("fatal message JJSjaamcng");
  }

private:
  NFD_LOG_INCLASS_DECLARE();
};

NFD_LOG_INCLASS_TEMPLATE_DEFINE(InClassTemplateLogger, "GenericInClassTemplateLogger");
NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(InClassTemplateLogger, int, "IntInClassLogger");

BOOST_FIXTURE_TEST_CASE(GenericInTemplatedClass, InClassTemplateLogger<bool>)
{
  writeLogs();

  BOOST_CHECK_EQUAL(m_buffer.str(),
                    "TRACE: [GenericInClassTemplateLogger] trace message JHGFDSR^1\n"
                    "DEBUG: [GenericInClassTemplateLogger] debug message IGg2474fdksd fo 151617\n"
                    "WARNING: [GenericInClassTemplateLogger] warning message XXXhdhd111x\n"
                    "INFO: [GenericInClassTemplateLogger] info message Jjxjshj13\n"
                    "ERROR: [GenericInClassTemplateLogger] error message !#$&^%$#@\n"
                    "FATAL: [GenericInClassTemplateLogger] fatal message JJSjaamcng\n"
                    );
}

BOOST_FIXTURE_TEST_CASE(SpecializedInTemplatedClass, InClassTemplateLogger<int>)
{
  writeLogs();

  BOOST_CHECK_EQUAL(m_buffer.str(),
                    "TRACE: [IntInClassLogger] trace message JHGFDSR^1\n"
                    "DEBUG: [IntInClassLogger] debug message IGg2474fdksd fo 151617\n"
                    "WARNING: [IntInClassLogger] warning message XXXhdhd111x\n"
                    "INFO: [IntInClassLogger] info message Jjxjshj13\n"
                    "ERROR: [IntInClassLogger] error message !#$&^%$#@\n"
                    "FATAL: [IntInClassLogger] fatal message JJSjaamcng\n"
                    );
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
