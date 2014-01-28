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

BOOST_AUTO_TEST_CASE(Warn)
{
  std::stringstream input;
  input << "warnTest";
  
  std::stringstream buffer;

  // save cerr's buffer here
  std::streambuf* sbuf = std::cerr.rdbuf();

  // redirect cerr to the buffer
  std::cerr.rdbuf(buffer.rdbuf());

  NFD_LOG_INIT("WarnTest");
  NFD_LOG_WARN(input);
  
  // redirect cerr back
  std::cerr.rdbuf(sbuf);
  
  std::stringstream trueValue;
  trueValue << "WARNING: [WarnTest] " << input <<"\n";
  
  BOOST_CHECK_EQUAL(buffer.str(), trueValue.str());
}

BOOST_AUTO_TEST_CASE(Debug)
{
  std::stringstream input;
  input << "debugTest";
  
  std::stringstream buffer;

  // save cerr's buffer here
  std::streambuf* sbuf = std::cerr.rdbuf();

  // redirect cerr to the buffer
  std::cerr.rdbuf(buffer.rdbuf());

  NFD_LOG_INIT("DebugTest");
  NFD_LOG_DEBUG(input);
  
  // redirect cerr back
  std::cerr.rdbuf(sbuf);
  
  std::stringstream trueValue;
  trueValue << "DEBUG: [DebugTest] " << input <<"\n";
  
  BOOST_CHECK_EQUAL(buffer.str(), trueValue.str());
}

BOOST_AUTO_TEST_CASE(Info)
{
  std::stringstream input;
  input << "infotest";
  
  std::stringstream buffer;

  // save cerr's buffer here
  std::streambuf* sbuf = std::cerr.rdbuf();

  // redirect cerr to the buffer
  std::cerr.rdbuf(buffer.rdbuf());

  NFD_LOG_INIT("InfoTest");
  NFD_LOG_INFO(input);
  
  // redirect cerr back
  std::cerr.rdbuf(sbuf);
  
  std::stringstream trueValue;
  trueValue << "INFO: [InfoTest] " << input <<"\n";
  
  BOOST_CHECK_EQUAL(buffer.str(), trueValue.str());
}

BOOST_AUTO_TEST_CASE(Fatal)
{
  std::stringstream input;
  input << "fataltest";
  
  std::stringstream buffer;

  // save cerr's buffer here
  std::streambuf* sbuf = std::cerr.rdbuf();

  // redirect cerr to the buffer
  std::cerr.rdbuf(buffer.rdbuf());

  NFD_LOG_INIT("FatalTest");
  NFD_LOG_FATAL(input);
  
  // redirect cerr back
  std::cerr.rdbuf(sbuf);
  
  std::stringstream trueValue;
  trueValue << "FATAL: [FatalTest] " << input <<"\n";
  
  BOOST_CHECK_EQUAL(buffer.str(), trueValue.str());
}

BOOST_AUTO_TEST_CASE(Error)
{
  std::stringstream input;
  input << "errortest";
  
  std::stringstream buffer;

  // save cerr's buffer here
  std::streambuf* sbuf = std::cerr.rdbuf();

  // redirect cerr to the buffer
  std::cerr.rdbuf(buffer.rdbuf());

  NFD_LOG_INIT("ErrorTest");
  NFD_LOG_ERROR(input);
  
  // redirect cerr back
  std::cerr.rdbuf(sbuf);
  
  std::stringstream trueValue;
  trueValue << "ERROR: [ErrorTest] " << input <<"\n";
  
  BOOST_CHECK_EQUAL(buffer.str(), trueValue.str());
}

BOOST_AUTO_TEST_CASE(Trace)
{
  std::stringstream input;
  input << "tracetest";
  
  std::stringstream buffer;

  // save cerr's buffer here
  std::streambuf* sbuf = std::cerr.rdbuf();

  // redirect cerr to the buffer
  std::cerr.rdbuf(buffer.rdbuf());

  NFD_LOG_INIT("TraceTest");
  NFD_LOG_TRACE(input);
  
  // redirect cerr back
  std::cerr.rdbuf(sbuf);
  
  std::stringstream trueValue;
  trueValue << "TRACE: [TraceTest] " << input <<"\n";
  
  BOOST_CHECK_EQUAL(buffer.str(), trueValue.str());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
