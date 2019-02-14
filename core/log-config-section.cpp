/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "log-config-section.hpp"

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/logging.hpp>

namespace nfd {
namespace log {

static ndn::util::LogLevel
parseLogLevel(const ConfigSection& item, const std::string& key)
{
  try {
    return ndn::util::parseLogLevel(item.get_value<std::string>());
  }
  catch (const std::invalid_argument&) {
    NDN_THROW_NESTED(ConfigFile::Error("Invalid log level for '" + key + "' in section 'log'"));
  }
}

static void
onConfig(const ConfigSection& section, bool isDryRun, const std::string&)
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

  auto defaultLevel = ndn::util::LogLevel::INFO;
  auto item = section.get_child_optional("default_level");
  if (item) {
    defaultLevel = parseLogLevel(*item, "default_level");
  }
  if (!isDryRun) {
    // default_level applies only to NFD loggers
    ndn::util::Logging::setLevel("nfd.*", defaultLevel);
  }

  for (const auto& i : section) {
    if (i.first == "default_level") {
      // do nothing
    }
    else {
      auto level = parseLogLevel(i.second, i.first);
      if (!isDryRun) {
        if (i.first.find('.') == std::string::npos)
          // backward compat: assume unqualified logger names refer to NFD loggers
          ndn::util::Logging::setLevel("nfd." + i.first, level);
        else
          ndn::util::Logging::setLevel(i.first, level);
      }
    }
  }
}

void
setConfigFile(ConfigFile& config)
{
  config.addSectionHandler("log", &onConfig);
}

} // namespace log
} // namespace nfd
