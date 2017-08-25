/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "logger-factory.hpp"

#include <ndn-cxx/util/logging.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/range/adaptor/map.hpp>

#include <iostream>

#ifdef HAVE_CUSTOM_LOGGER
#error "This file should not be compiled when custom logger is used"
#endif

namespace nfd {

NFD_LOG_INIT("LoggerFactory");

LoggerFactory&
LoggerFactory::getInstance()
{
  static LoggerFactory globalLoggerFactory;

  return globalLoggerFactory;
}

LoggerFactory::LoggerFactory()
  : m_defaultLevel(LOG_INFO)
{
  m_levelNames["NONE"] = LOG_NONE;
  m_levelNames["ERROR"] = LOG_ERROR;
  m_levelNames["WARN"] = LOG_WARN;
  m_levelNames["INFO"] = LOG_INFO;
  m_levelNames["DEBUG"] = LOG_DEBUG;
  m_levelNames["TRACE"] = LOG_TRACE;
  m_levelNames["ALL"] = LOG_ALL;

  // Let ndn-cxx logging facility initialize Boost.Log backend,
  // so that only one sink is attached to Boost.Log core.
  ndn::util::Logging::setDestination(std::clog);
}

LoggerFactory::~LoggerFactory()
{
  ndn::util::Logging::flush();
}

void
LoggerFactory::setConfigFile(ConfigFile& config)
{
  config.addSectionHandler("log", bind(&LoggerFactory::onConfig, this, _1, _2, _3));
}

LogLevel
LoggerFactory::parseLevel(const std::string& level)
{
  std::string upperLevel = boost::to_upper_copy(level);

  // std::cerr << "parsing level: " << upperLevel << std::endl;;
  // std::cerr << "# levels: " << m_levelNames.size() << std::endl;
  // std::cerr << m_levelNames.begin()->first << std::endl;

  LevelMap::const_iterator levelIt = m_levelNames.find(upperLevel);
  if (levelIt != m_levelNames.end()) {
    return levelIt->second;
  }
  try {
    uint32_t levelNo = boost::lexical_cast<uint32_t>(level);

    if ((boost::lexical_cast<uint32_t>(LOG_NONE) <= levelNo &&
         levelNo <= boost::lexical_cast<uint32_t>(LOG_TRACE)) ||
        levelNo == LOG_ALL) {
      return static_cast<LogLevel>(levelNo);
    }
  }
  catch (const boost::bad_lexical_cast& error) {
  }

  BOOST_THROW_EXCEPTION(LoggerFactory::Error("Unsupported logging level \"" + level + "\""));
}

LogLevel
LoggerFactory::extractLevel(const ConfigSection& item, const std::string& key)
{
  std::string levelString;
  try {
    levelString = item.get_value<std::string>();
  }
  catch (const boost::property_tree::ptree_error& error) {
  }

  if (levelString.empty()) {
    BOOST_THROW_EXCEPTION(LoggerFactory::Error("No logging level found for option \"" + key + "\""));
  }

  return parseLevel(levelString);
}

void
LoggerFactory::onConfig(const ConfigSection& section,
                        bool isDryRun,
                        const std::string& filename)
{
  // log
  // {
  //   ; default_level specifies the logging level for modules
  //   ; that are not explicitly named. All debugging levels
  //   ; listed above the selected value are enabled.
  //
  //   default_level INFO
  //
  //   ; You may also override the default for specific modules:
  //
  //   FibManager DEBUG
  //   Forwarder WARN
  // }

  if (!isDryRun) {
    ConfigSection::const_assoc_iterator item = section.find("default_level");
    if (item != section.not_found()) {
      LogLevel level = extractLevel(item->second, "default_level");
      setDefaultLevel(level);
    }
    else {
      setDefaultLevel(LOG_INFO);
    }
  }

  for (const auto& i : section) {
    LogLevel level = extractLevel(i.second, i.first);

    if (i.first == "default_level") {
      // do nothing
    }
    else {
      std::unique_lock<std::mutex> lock(m_loggersGuard);
      LoggerMap::iterator loggerIt = m_loggers.find(i.first);
      if (loggerIt == m_loggers.end()) {
        lock.unlock();
        NFD_LOG_DEBUG("Failed to configure logging level for module \"" <<
                      i.first << "\" (module not found)");
      }
      else if (!isDryRun) {
        loggerIt->second.setLogLevel(level);
        lock.unlock();
        NFD_LOG_DEBUG("Changing level for module " << i.first << " to " << level);
      }
    }
  }
}

void
LoggerFactory::setDefaultLevel(LogLevel level)
{
  // std::cerr << "changing to default_level " << level << std::endl;
  std::lock_guard<std::mutex> lock(m_loggersGuard);

  m_defaultLevel = level;
  for (auto&& logger : m_loggers) {
    // std::cerr << "changing " << i->first << " to default " << m_defaultLevel << std::endl;
    logger.second.setLogLevel(m_defaultLevel);
  }
}

void
LoggerFactory::flushBackend()
{
  ndn::util::Logging::flush();
}

Logger&
LoggerFactory::create(const std::string& moduleName)
{
  return LoggerFactory::getInstance().createLogger(moduleName);
}

Logger&
LoggerFactory::createLogger(const std::string& moduleName)
{
  // std::cerr << "creating logger for " << moduleName
  //           << " with level " << m_defaultLevel << std::endl;

  std::lock_guard<std::mutex> lock(m_loggersGuard);

  std::pair<LoggerMap::iterator, bool> loggerIt =
    m_loggers.insert(NameAndLogger(moduleName, Logger(moduleName, m_defaultLevel)));

  return loggerIt.first->second;
}

std::list<std::string>
LoggerFactory::getModules() const
{
  std::lock_guard<std::mutex> lock(m_loggersGuard);

  std::list<std::string> modules;
  for (const auto& loggerName : m_loggers | boost::adaptors::map_keys) {
    modules.push_back(loggerName);
  }

  return modules;
}

} // namespace nfd
