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

#ifndef NFD_CORE_LOGGER_HPP
#define NFD_CORE_LOGGER_HPP

#include "common.hpp"

#ifdef HAVE_CUSTOM_LOGGER
#include "custom-logger.hpp"
#else

#include <mutex>

namespace nfd {

/** \brief indicates a log level
 *  \note This type is internal. Logger should be accessed through NFD_LOG_* macros.
 */
enum LogLevel {
  LOG_FATAL          = -1, // fatal (will be logged unconditionally)
  LOG_NONE           = 0, // no messages
  LOG_ERROR          = 1, // serious error messages
  LOG_WARN           = 2, // warning messages
  LOG_INFO           = 3, // informational messages
  LOG_DEBUG          = 4, // debug messages
  LOG_TRACE          = 5, // trace messages (most verbose)
  LOG_ALL            = 255 // all messages
};

/** \brief provides logging for a module
 *  \note This type is internal. Logger should be accessed through NFD_LOG_* macros.
 *  \note This type is copyable because logger can be declared as a field of
 *        (usually template) classes, and shouldn't prevent those classes to be copyable.
 */
class Logger
{
public:
  Logger(const std::string& name, LogLevel level);

  bool
  isEnabled(LogLevel level) const
  {
    return m_enabledLogLevel >= level;
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

  /** \return string representation of time since epoch: seconds.microseconds
   *  \warning Return value is in a statically allocated buffer,
   *           which subsequent calls will overwrite.
   */
  static const char*
  now();

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

#define NFD_LOG_INIT(name) \
static nfd::Logger& g_logger = nfd::LoggerFactory::create(name)

#define NFD_LOG_INCLASS_DECLARE() \
static nfd::Logger& g_logger

#define NFD_LOG_INCLASS_DEFINE(cls, name) \
nfd::Logger& cls::g_logger = nfd::LoggerFactory::create(name)

#define NFD_LOG_INCLASS_TEMPLATE_DEFINE(cls, name) \
template<class T>                                  \
nfd::Logger& cls<T>::g_logger = nfd::LoggerFactory::create(name)

#define NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(cls, specialization, name) \
template<>                                                                        \
nfd::Logger& cls<specialization>::g_logger = nfd::LoggerFactory::create(name)

#define NFD_LOG_INCLASS_2TEMPLATE_SPECIALIZATION_DEFINE(cls, s1, s2, name) \
template<>                                                                 \
nfd::Logger& cls<s1, s2>::g_logger = nfd::LoggerFactory::create(name)

extern std::mutex g_logMutex;

#define NFD_LOG(level, msg, expression)                          \
do {                                                             \
  if (g_logger.isEnabled(::nfd::LOG_##level)) {                  \
    std::lock_guard<std::mutex> lock(::nfd::g_logMutex);         \
    std::clog << ::nfd::Logger::now() << " "#msg": "             \
              << "[" << g_logger << "] " << expression << "\n";  \
  }                                                              \
} while (false)

#define NFD_LOG_TRACE(expression) NFD_LOG(TRACE, TRACE,   expression)
#define NFD_LOG_DEBUG(expression) NFD_LOG(DEBUG, DEBUG,   expression)
#define NFD_LOG_INFO(expression)  NFD_LOG(INFO,  INFO,    expression)
#define NFD_LOG_WARN(expression)  NFD_LOG(WARN,  WARNING, expression)
#define NFD_LOG_ERROR(expression) NFD_LOG(ERROR, ERROR,   expression)
#define NFD_LOG_FATAL(expression) NFD_LOG(FATAL, FATAL,   expression)

} // namespace nfd

#endif // HAVE_CUSTOM_LOGGER

#endif // NFD_CORE_LOGGER_HPP
