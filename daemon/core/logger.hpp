/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#ifndef NFD_CORE_LOGGER_HPP
#define NFD_CORE_LOGGER_HPP

#include "common.hpp"
#include <iostream>

namespace nfd {

enum LogLevel {
  LOG_NONE           = 0, // no messages
  LOG_ERROR          = 1, // serious error messages
  LOG_WARN           = 2, // warning messages
  LOG_TRACE          = 3, // trace messages
  LOG_DEBUG          = 4, // debug messages
  LOG_INFO           = 5, // informational messages
  LOG_FATAL          = 6, // fatal error messages
  LOG_ALL            = 0x0fffffff, // all messages
};

class Logger
{
public:
  Logger(const std::string& name);
  
  bool
  isEnabled(LogLevel level)
  {
    return m_isEnabled;
  }
  
  const std::string&
  getName() const
  {
    return m_moduleName;
  }
  
private:
  std::string m_moduleName;
  bool m_isEnabled;
};

std::ostream&
operator<<(std::ostream& output, const Logger& obj);

#define NFD_LOG_INIT(name)    \
  static nfd::Logger \
  g_logger = nfd::Logger(name);

#define NFD_LOG_TRACE(expression) \
    if(g_logger.isEnabled(nfd::LOG_TRACE)) \
       std::cerr<<"TRACE: "<<"["<<g_logger<<"] " << expression << "\n"

#define NFD_LOG_DEBUG(expression)\
    if(g_logger.isEnabled(nfd::LOG_DEBUG)) \
       std::cerr<<"DEBUG: "<<"["<<g_logger<<"] " << expression <<"\n"

#define NFD_LOG_WARN(expression) \
    if(g_logger.isEnabled(nfd::LOG_WARN)) \
       std::cerr<<"WARNING: "<<"["<<g_logger<<"] " << expression <<"\n"

#define NFD_LOG_INFO(expression)\
    if(g_logger.isEnabled(nfd::LOG_INFO)) \
       std::cerr<<"INFO: "<<"["<<g_logger<<"] " << expression <<"\n"
  
#define NFD_LOG_ERROR(expression)\
    if(g_logger.isEnabled(nfd::LOG_ERROR)) \
       std::cerr<<"ERROR: "<<"["<<g_logger<<"] " << expression <<"\n"
  
#define NFD_LOG_FATAL(expression)\
    if(g_logger.isEnabled(nfd::LOG_FATAL)) \
       std::cerr<<"FATAL: "<<"["<<g_logger<<"] " << expression <<"\n"
  
} //namespace nfd


#endif
