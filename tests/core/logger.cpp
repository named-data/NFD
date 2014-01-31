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


namespace nfd {

BOOST_AUTO_TEST_SUITE(CoreLogger)

struct LoggerFixture
{
  LoggerFixture()
    : m_savedBuf(std::cerr.rdbuf())
  {
    std::cerr.rdbuf(m_buffer.rdbuf());
  }

  ~LoggerFixture()
  {
    std::cerr.rdbuf(m_savedBuf);
  }
  
  std::stringstream m_buffer;
  std::streambuf* m_savedBuf;
};

BOOST_FIXTURE_TEST_CASE(Basic, LoggerFixture)
{
  NFD_LOG_INIT("BasicTests");
  
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

class InClassLogger : public LoggerFixture
{
public:
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

} // namespace nfd
