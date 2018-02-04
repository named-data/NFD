/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "config-file.hpp"

#include <boost/property_tree/info_parser.hpp>
#include <fstream>
#include <sstream>

namespace nfd {

ConfigFile::ConfigFile(UnknownConfigSectionHandler unknownSectionCallback)
  : m_unknownSectionCallback(unknownSectionCallback)
{
}

void
ConfigFile::throwErrorOnUnknownSection(const std::string& filename,
                                       const std::string& sectionName,
                                       const ConfigSection& section,
                                       bool isDryRun)
{
  BOOST_THROW_EXCEPTION(Error("Error processing configuration file " + filename +
                              ": no module subscribed for section \"" + sectionName + "\""));
}

void
ConfigFile::ignoreUnknownSection(const std::string& filename,
                                 const std::string& sectionName,
                                 const ConfigSection& section,
                                 bool isDryRun)
{
  // do nothing
}

bool
ConfigFile::parseYesNo(const ConfigSection& node, const std::string& key,
                       const std::string& sectionName)
{
  auto value = node.get_value<std::string>();

  if (value == "yes") {
    return true;
  }
  else if (value == "no") {
    return false;
  }

  BOOST_THROW_EXCEPTION(Error("Invalid value \"" + value + "\" for option \"" +
                              key + "\" in \"" + sectionName + "\" section"));
}

void
ConfigFile::addSectionHandler(const std::string& sectionName,
                              ConfigSectionHandler subscriber)
{
  m_subscriptions[sectionName] = subscriber;
}

void
ConfigFile::parse(const std::string& filename, bool isDryRun)
{
  std::ifstream inputFile(filename);
  if (!inputFile.good() || !inputFile.is_open()) {
    BOOST_THROW_EXCEPTION(Error("Failed to read configuration file " + filename));
  }
  parse(inputFile, isDryRun, filename);
  inputFile.close();
}

void
ConfigFile::parse(const std::string& input, bool isDryRun, const std::string& filename)
{
  std::istringstream inputStream(input);
  parse(inputStream, isDryRun, filename);
}

void
ConfigFile::parse(std::istream& input, bool isDryRun, const std::string& filename)
{
  try {
    boost::property_tree::read_info(input, m_global);
  }
  catch (const boost::property_tree::info_parser_error& error) {
    BOOST_THROW_EXCEPTION(Error("Failed to parse configuration file " + filename +
                                ": " + error.message() + " on line " + to_string(error.line())));
  }

  process(isDryRun, filename);
}

void
ConfigFile::parse(const ConfigSection& config, bool isDryRun, const std::string& filename)
{
  m_global = config;
  process(isDryRun, filename);
}

void
ConfigFile::process(bool isDryRun, const std::string& filename) const
{
  BOOST_ASSERT(!filename.empty());

  for (const auto& i : m_global) {
    try {
      const ConfigSectionHandler& subscriber = m_subscriptions.at(i.first);
      subscriber(i.second, isDryRun, filename);
    }
    catch (const std::out_of_range&) {
      m_unknownSectionCallback(filename, i.first, i.second, isDryRun);
    }
  }
}

} // namespace nfd
