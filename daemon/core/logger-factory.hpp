/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_LOGGER_FACTORY_HPP
#define NFD_CORE_LOGGER_FACTORY_HPP

#include "common.hpp"
#include "mgmt/config-file.hpp"
#include "logger.hpp"

namespace nfd {

class LoggerFactory : noncopyable
{
public:

  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& error)
      : std::runtime_error(error)
    {
    }
  };

  static LoggerFactory&
  getInstance();

  void
  setConfigFile(ConfigFile& config);

  void
  onConfig(const ConfigSection& section, bool isDryRun, const std::string& filename);

  std::list<std::string>
  getModules() const;

  static Logger&
  create(const std::string& moduleName);


PUBLIC_WITH_TESTS_ELSE_PRIVATE:

  // these methods are used during unit-testing

  LogLevel
  getDefaultLevel() const;

  void
  setDefaultLevel(LogLevel level);

private:

  LoggerFactory();

  Logger&
  createLogger(const std::string& moduleName);

  LogLevel
  parseLevel(const std::string& level);

private:

  typedef std::map<std::string, LogLevel> LevelMap;
  typedef std::pair<std::string, LogLevel> NameAndLevel;

  LevelMap m_levelNames;

  typedef std::map<std::string, Logger> LoggerMap;
  typedef std::pair<std::string, Logger> NameAndLogger;

  LoggerMap m_loggers;

  LogLevel m_defaultLevel;
};

inline LogLevel
LoggerFactory::getDefaultLevel() const
{
  return m_defaultLevel;
}

} // namespace nfd

#endif // NFD_CORE_LOGGER_FACTORY_HPP
