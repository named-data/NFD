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
#include <ndn-cpp-dev/util/time.hpp>

/// \todo use when we enable C++11 (see todo in now())
// #include <cinttypes>

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


  /// \brief return a string representation of time since epoch: seconds.microseconds
  static std::string
  now()
  {
    using namespace ndn::time;

    static const microseconds::rep ONE_SECOND = 1000000;

    // 10 (whole seconds) + '.' + 6 (fraction) + 1 (\0)
    char buffer[10 + 1 + 6 + 1];

    microseconds::rep microseconds_since_epoch =
      duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

    ::snprintf(buffer, sizeof(buffer), "%lld.%06lld",
               static_cast<long long int>(microseconds_since_epoch / ONE_SECOND),
               static_cast<long long int>(microseconds_since_epoch % ONE_SECOND));

    /// \todo use this version when we enable C++11 to avoid casting
    // ::snprintf(buffer, sizeof(buffer), "%" PRIdLEAST64 ".%06" PRIdLEAST64,
    //            microseconds_since_epoch / ONE_SECOND,
    //            microseconds_since_epoch % ONE_SECOND);

    return std::string(buffer);
  }

private:
  std::string m_moduleName;
  LogLevel    m_enabledLogLevel;
};

inline std::ostream&
operator<<(std::ostream& output, const Logger& logger)
{
  output << logger.getName();
  return output;
}

} // namespace nfd

#include "core/logger-factory.hpp"

namespace nfd {

#define NFD_LOG_INIT(name)                                              \
static nfd::Logger& g_logger = nfd::LoggerFactory::create(name);

#define NFD_LOG_INCLASS_DECLARE()               \
static nfd::Logger& g_logger;

#define NFD_LOG_INCLASS_DEFINE(cls, name)                       \
nfd::Logger& cls::g_logger = nfd::LoggerFactory::create(name);

#define NFD_LOG_INCLASS_TEMPLATE_DEFINE(cls, name)                      \
template<class T>                                                       \
nfd::Logger& cls<T>::g_logger = nfd::LoggerFactory::create(name);

#define NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(cls, specialization, name) \
template<>                                                              \
nfd::Logger& cls<specialization>::g_logger = nfd::LoggerFactory::create(name);

#define NFD_LOG_INCLASS_2TEMPLATE_SPECIALIZATION_DEFINE(cls, s1, s2, name) \
template<>                                                              \
nfd::Logger& cls<s1, s2>::g_logger = nfd::LoggerFactory::create(name);


#define NFD_LOG(level, expression)                                      \
do {                                                                    \
  if (g_logger.isEnabled(::nfd::LOG_##level))                           \
    std::cerr << ::nfd::Logger::now() << " "#level": "                  \
              << "[" << g_logger << "] " << expression << "\n";         \
} while (false)

#define NFD_LOG_TRACE(expression) NFD_LOG(TRACE, expression)
#define NFD_LOG_DEBUG(expression) NFD_LOG(DEBUG, expression)
#define NFD_LOG_INFO(expression) NFD_LOG(INFO, expression)
#define NFD_LOG_ERROR(expression) NFD_LOG(ERROR, expression)

// specialize WARN because the message is "WARNING" instead of "WARN"
#define NFD_LOG_WARN(expression)                                        \
do {                                                                    \
  if (g_logger.isEnabled(::nfd::LOG_WARN))                              \
    std::cerr << ::nfd::Logger::now() << " WARNING: "                   \
              << "[" << g_logger << "] " << expression << "\n";         \
} while (false)

#define NFD_LOG_FATAL(expression)                                       \
do {                                                                    \
  std::cerr << ::nfd::Logger::now() << " FATAL: "                       \
            << "[" << g_logger << "] " << expression << "\n";           \
} while (false)

} //namespace nfd

#endif
