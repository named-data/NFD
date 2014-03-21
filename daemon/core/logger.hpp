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

namespace nfd {

enum LogLevel {
  LOG_NONE           = 0, // no messages
  LOG_ERROR          = 1, // serious error messages
  LOG_WARN           = 2, // warning messages
  LOG_INFO           = 3, // informational messages
  LOG_DEBUG          = 4, // debug messages
  LOG_TRACE          = 5, // trace messages (most verbose)
  // LOG_FATAL is not a level and is logged unconditionally
  LOG_ALL            = 255, // all messages
};

class Logger
{
public:

  Logger(const std::string& name, LogLevel level)
    : m_moduleName(name)
    , m_enabledLogLevel(level)
  {
  }

  bool
  isEnabled(LogLevel level) const
  {
    // std::cerr << m_moduleName <<
    //   " enabled = " << m_enabledLogLevel
    //           << " level = " << level << std::endl;
    return (m_enabledLogLevel >= level);
  }

  void
  setLogLevel(LogLevel level)
  {
    m_enabledLogLevel = level;
  }

  const std::string&
  getName() const
  {
    return m_moduleName;
  }

  void
  setName(const std::string& name)
  {
    m_moduleName = name;
  }

private:
  std::string m_moduleName;
  LogLevel    m_enabledLogLevel;
};

inline std::ostream&
operator<<(std::ostream& output, const Logger& obj)
{
  output << obj.getName();
  return output;
}

} // namespace nfd

#include "core/logger-factory.hpp"

namespace nfd {

#define NFD_LOG_INIT(name) \
static nfd::Logger& g_logger = nfd::LoggerFactory::create(name);

#define NFD_LOG_INCLASS_DECLARE()        \
static nfd::Logger& g_logger;

#define NFD_LOG_INCLASS_DEFINE(cls, name)        \
nfd::Logger& cls::g_logger = nfd::LoggerFactory::create(name);

#define NFD_LOG_INCLASS_TEMPLATE_DEFINE(cls, name)   \
template<class T>                                                       \
nfd::Logger& cls<T>::g_logger = nfd::LoggerFactory::create(name);

#define NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(cls, specialization, name) \
template<> \
nfd::Logger& cls<specialization>::g_logger = nfd::LoggerFactory::create(name);

#define NFD_LOG_INCLASS_2TEMPLATE_SPECIALIZATION_DEFINE(cls, s1, s2, name) \
template<>                                                              \
nfd::Logger& cls<s1, s2>::g_logger = nfd::LoggerFactory::create(name);


#define NFD_LOG_TRACE(expression) \
if (g_logger.isEnabled(nfd::LOG_TRACE))                                 \
  std::cerr<<"TRACE: "<<"["<<g_logger<<"] " << expression << "\n"

#define NFD_LOG_DEBUG(expression)\
if (g_logger.isEnabled(nfd::LOG_DEBUG))                                 \
  std::cerr<<"DEBUG: "<<"["<<g_logger<<"] " << expression <<"\n"

#define NFD_LOG_WARN(expression)                                        \
if (g_logger.isEnabled(nfd::LOG_WARN))                                  \
  std::cerr<<"WARNING: "<<"["<<g_logger<<"] " << expression <<"\n"

#define NFD_LOG_INFO(expression)\
if (g_logger.isEnabled(nfd::LOG_INFO))                          \
  std::cerr<<"INFO: "<<"["<<g_logger<<"] " << expression <<"\n"

#define NFD_LOG_ERROR(expression)                                       \
if (g_logger.isEnabled(nfd::LOG_ERROR))                                  \
  std::cerr<<"ERROR: "<<"["<<g_logger<<"] " << expression <<"\n"

#define NFD_LOG_FATAL(expression)\
std::cerr<<"FATAL: "<<"["<<g_logger<<"] " << expression <<"\n"

} //namespace nfd

#endif
