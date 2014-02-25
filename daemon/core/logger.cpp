/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#include "logger.hpp"
#include <boost/algorithm/string.hpp>

namespace nfd {

inline static bool
isNumber(const std::string& s)
{
  if (s.empty())
    return false;

  for (std::string::const_iterator i = s.begin(); i != s.end(); ++i)
    {
      if (!std::isdigit(*i))
        return false;
    }

  return true;
}

Logger::Logger(const std::string& name)
  : m_moduleName(name)
{
  char* nfdLog = getenv("NFD_LOG");
  if (nfdLog)
    {
      std::string level = nfdLog;
      if (isNumber(nfdLog))
        {
          setLogLevel(boost::lexical_cast<uint32_t>(nfdLog));
        }
      else if (boost::iequals(level, "none"))
        {
          m_enabledLogLevel = LOG_NONE;
        }
      else if (boost::iequals(level, "error"))
        {
          m_enabledLogLevel = LOG_ERROR;
        }
      else if (boost::iequals(level, "warn"))
        {
          m_enabledLogLevel = LOG_WARN;
        }
      else if (boost::iequals(level, "info"))
        {
          m_enabledLogLevel = LOG_INFO;
        }
      else if (boost::iequals(level, "debug"))
        {
          m_enabledLogLevel = LOG_DEBUG;
        }
      else if (boost::iequals(level, "trace"))
        {
          m_enabledLogLevel = LOG_TRACE;
        }
      else
        {
          m_enabledLogLevel = LOG_ALL;
        }
    }
  else
    {
      m_enabledLogLevel = LOG_WARN;
    }
}

std::ostream&
operator<<(std::ostream& output, const Logger& obj)
{
  output << obj.getName();
  return output;
}

} // namespace nfd
