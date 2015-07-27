/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "config-file.hpp"
#include "logger.hpp"

#include <boost/property_tree/info_parser.hpp>
#include <fstream>

namespace nfd {

void
ConfigFile::throwErrorOnUnknownSection(const std::string& filename,
                                       const std::string& sectionName,
                                       const ConfigSection& section,
                                       bool isDryRun)
{
  std::string msg = "Error processing configuration file ";
  msg += filename;
  msg += ": no module subscribed for section \"" + sectionName + "\"";

  BOOST_THROW_EXCEPTION(ConfigFile::Error(msg));
}

void
ConfigFile::ignoreUnknownSection(const std::string& filename,
                                 const std::string& sectionName,
                                 const ConfigSection& section,
                                 bool isDryRun)
{
  // do nothing
}

ConfigFile::ConfigFile(UnknownConfigSectionHandler unknownSectionCallback)
  : m_unknownSectionCallback(unknownSectionCallback)
{
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
  std::ifstream inputFile;
  inputFile.open(filename.c_str());
  if (!inputFile.good() || !inputFile.is_open())
    {
      std::string msg = "Failed to read configuration file: ";
      msg += filename;
      BOOST_THROW_EXCEPTION(Error(msg));
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
  try
    {
      boost::property_tree::read_info(input, m_global);
    }
  catch (const boost::property_tree::info_parser_error& error)
    {
      std::stringstream msg;
      msg << "Failed to parse configuration file";
      msg << " " << filename;
      msg << " " << error.message() << " line " << error.line();
      BOOST_THROW_EXCEPTION(Error(msg.str()));
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
ConfigFile::process(bool isDryRun, const std::string& filename)
{
  BOOST_ASSERT(!filename.empty());
  // NFD_LOG_DEBUG("processing..." << ((isDryRun)?("dry run"):("")));

  if (m_global.begin() == m_global.end())
    {
      std::string msg = "Error processing configuration file: ";
      msg += filename;
      msg += " no data";
      BOOST_THROW_EXCEPTION(Error(msg));
    }

  for (ConfigSection::const_iterator i = m_global.begin(); i != m_global.end(); ++i)
    {
      const std::string& sectionName = i->first;
      const ConfigSection& section = i->second;

      SubscriptionTable::iterator subscriberIt = m_subscriptions.find(sectionName);
      if (subscriberIt != m_subscriptions.end())
        {
          ConfigSectionHandler subscriber = subscriberIt->second;
          subscriber(section, isDryRun, filename);
        }
      else
        {
          m_unknownSectionCallback(filename, sectionName, section, isDryRun);
        }
    }
}

}
