/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */


#include "config-file.hpp"

#include <boost/property_tree/info_parser.hpp>

namespace nfd {

NFD_LOG_INIT("ConfigFile");

ConfigFile::ConfigFile()
{

}

void
ConfigFile::addSectionHandler(const std::string& sectionName,
                              OnConfig subscriber)
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
      throw Error(msg);
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
      throw Error(msg.str());
    }

  process(isDryRun, filename);
}

void
ConfigFile::process(bool isDryRun, const std::string& filename)
{
  BOOST_ASSERT(!filename.empty());
  // NFD_LOG_DEBUG("processing..." << ((isDryRun)?("dry run"):("")));

  if (m_global.begin() == m_global.end())
    {
      std::string msg = "Error processing configuration file";
      msg += ": ";
      msg += filename;
      msg += " no data";
      throw Error(msg);
    }

  for (ConfigSection::const_iterator i = m_global.begin(); i != m_global.end(); ++i)
    {
      const std::string& sectionName = i->first;
      const ConfigSection& section = i->second;

      SubscriptionTable::iterator subscriberIt = m_subscriptions.find(sectionName);
      if (subscriberIt != m_subscriptions.end())
        {
          OnConfig subscriber = subscriberIt->second;
          subscriber(section, isDryRun);
        }
      else
        {
          std::string msg = "Error processing configuration file";
          msg += " ";
          msg += filename;
          msg += " no module subscribed for section: " + sectionName;
          throw Error(msg);
        }
    }
}

}

