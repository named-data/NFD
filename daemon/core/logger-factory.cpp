/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "logger-factory.hpp"

namespace nfd {

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
}

void
LoggerFactory::setConfigFile(ConfigFile& config)
{
  config.addSectionHandler("log", bind(&LoggerFactory::onConfig, this, _1, _2, _3));
}

LogLevel
LoggerFactory::parseLevel(const std::string& level)
{
  std::string upperLevel = level;
  boost::to_upper(upperLevel);

  // std::cerr << "parsing level: " << upperLevel << std::endl;;
  // std::cerr << "# levels: " << m_levelNames.size() << std::endl;
  // std::cerr << m_levelNames.begin()->first << std::endl;

  LevelMap::const_iterator levelIt = m_levelNames.find(upperLevel);
  if (levelIt != m_levelNames.end())
    {
      return levelIt->second;
    }
  try
    {
      uint32_t levelNo = boost::lexical_cast<uint32_t>(level);

      if ((LOG_NONE <= levelNo && levelNo <= LOG_TRACE) ||
          levelNo == LOG_ALL)
        {
          return static_cast<LogLevel>(levelNo);
        }
    }
  catch (const boost::bad_lexical_cast& error)
    {
    }
  throw LoggerFactory::Error("Unsupported logging level \"" +
                             level + "\"");
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

  // std::cerr << "loading logging configuration" << std::endl;
  for (ConfigSection::const_iterator item = section.begin();
       item != section.end();
       ++item)
    {
      std::string levelString;
      try
        {
          levelString = item->second.get_value<std::string>();
        }
      catch (const boost::property_tree::ptree_error& error)
        {
        }

      if (levelString.empty())
        {
          throw LoggerFactory::Error("No logging level found for option \"" + item->first + "\"");
        }

      LogLevel level = parseLevel(levelString);

      if (item->first == "default_level")
        {
          if (!isDryRun)
            {
              setDefaultLevel(level);
            }
        }
      else
        {
          LoggerMap::iterator loggerIt = m_loggers.find(item->first);
          if (loggerIt == m_loggers.end())
            {
              throw LoggerFactory::Error("Invalid module name \"" +
                                         item->first + "\" in configuration file");
            }

          if (!isDryRun)
            {
              // std::cerr << "changing level for module " << item->first << " to " << level << std::endl;
              loggerIt->second.setLogLevel(level);
            }
        }
    }
}

void
LoggerFactory::setDefaultLevel(LogLevel level)
{
  // std::cerr << "changing to default_level " << level << std::endl;

  m_defaultLevel = level;
  for (LoggerMap::iterator i = m_loggers.begin(); i != m_loggers.end(); ++i)
    {
      // std::cerr << "changing " << i->first << " to default " << m_defaultLevel << std::endl;
      i->second.setLogLevel(m_defaultLevel);
    }
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

  std::pair<LoggerMap::iterator, bool> loggerIt =
    m_loggers.insert(NameAndLogger(moduleName, Logger(moduleName, m_defaultLevel)));

  return loggerIt.first->second;
}

std::list<std::string>
LoggerFactory::getModules() const
{
  std::list<std::string> modules;
  for (LoggerMap::const_iterator i = m_loggers.begin(); i != m_loggers.end(); ++i)
    {
      modules.push_back(i->first);
    }

  return modules;
}

} // namespace nfd
