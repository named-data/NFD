/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "command-validator.hpp"
#include <ndn-cpp-dev/util/io.hpp>
#include <ndn-cpp-dev/security/identity-certificate.hpp>

namespace nfd {

NFD_LOG_INIT("CommandValidator");

CommandValidator::CommandValidator()
{

}

CommandValidator::~CommandValidator()
{

}

void
CommandValidator::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("authorizations",
                               bind(&CommandValidator::onConfig, this, _1, _2));
}

static inline void
aggregateErrors(std::stringstream& ss, const std::string& msg)
{
  if (!ss.str().empty())
    {
      ss << "\n";
    }
  ss << msg;
}

void
CommandValidator::onConfig(const ConfigSection& section,
                           bool isDryRun)
{
  const ConfigSection EMPTY_SECTION;

  if (section.begin() == section.end())
    {
      throw ConfigFile::Error("No authorize sections found");
    }

  std::stringstream dryRunErrors;
  ConfigSection::const_iterator authIt;
  for (authIt = section.begin(); authIt != section.end(); authIt++)
    {
      std::string keyfile;
      try
        {
          keyfile = authIt->second.get<std::string>("keyfile");
        }
      catch (const std::runtime_error& e)
        {
          std::string msg = "No keyfile specified";
          if (!isDryRun)
            {
              throw ConfigFile::Error(msg);
            }
          aggregateErrors(dryRunErrors, msg);
          continue;
        }

      std::ifstream in;
      in.open(keyfile.c_str());
      if (!in.is_open())
        {
          std::string msg = "Unable to open key file " + keyfile;
          if (!isDryRun)
            {
              throw ConfigFile::Error(msg);
            }
          aggregateErrors(dryRunErrors, msg);
          continue;
        }

      shared_ptr<ndn::IdentityCertificate> id;
      try
        {
          id = ndn::io::load<ndn::IdentityCertificate>(in);
        }
      catch(const std::runtime_error& error)
        {
          std::string msg = "Malformed key file " + keyfile;
          if (!isDryRun)
            {
              throw ConfigFile::Error(msg);
            }
          aggregateErrors(dryRunErrors, msg);
          continue;
        }

      in.close();

      const ConfigSection* privileges = 0;

      try
        {
          privileges = &authIt->second.get_child("privileges");
        }
      catch (const std::runtime_error& error)
        {
          std::string msg = "No privileges section found for key file " +
            keyfile + " (" + id->getPublicKeyName().toUri() + ")";
          if (!isDryRun)
            {
              throw ConfigFile::Error(msg);
            }
          aggregateErrors(dryRunErrors, msg);
          continue;
        }

      if (privileges->begin() == privileges->end())
        {
          NFD_LOG_WARN("No privileges specified for key file " << keyfile + " (" << id->getPublicKeyName().toUri() << ")");
        }

      ConfigSection::const_iterator privIt;
      for (privIt = privileges->begin(); privIt != privileges->end(); privIt++)
        {
          const std::string& privilegeName = privIt->first;
          if (m_supportedPrivileges.find(privilegeName) != m_supportedPrivileges.end())
            {
              NFD_LOG_INFO("Giving privilege \"" << privilegeName
                           << "\" to key " << id->getPublicKeyName());
              if (!isDryRun)
                {
                  const std::string regex = "^<localhost><nfd><" + privilegeName + ">";
                  m_validator.addInterestRule(regex, *id);
                }
            }
          else
            {
              // Invalid configuration
              std::string msg = "Invalid privilege \"" + privilegeName + "\" for key file " +
                keyfile + " (" + id->getPublicKeyName().toUri() + ")";
              if (!isDryRun)
                {
                  throw ConfigFile::Error(msg);
                }
              aggregateErrors(dryRunErrors, msg);
            }
        }
    }

  if (!dryRunErrors.str().empty())
    {
      throw ConfigFile::Error(dryRunErrors.str());
    }
}

void
CommandValidator::addSupportedPrivilege(const std::string& privilege)
{
  if (m_supportedPrivileges.find(privilege) != m_supportedPrivileges.end())
    {
      throw CommandValidator::Error("Duplicated privivilege: " + privilege);
    }
  m_supportedPrivileges.insert(privilege);
}

} // namespace nfd

