/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "command-validator.hpp"
#include "core/logger.hpp"

#include <ndn-cpp-dev/util/io.hpp>
#include <ndn-cpp-dev/security/identity-certificate.hpp>

#include <boost/filesystem.hpp>

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
                               bind(&CommandValidator::onConfig, this, _1, _2, _3));
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
                           bool isDryRun,
                           const std::string& filename)
{
  using namespace boost::filesystem;

  const ConfigSection EMPTY_SECTION;

  if (section.begin() == section.end())
    {
      throw ConfigFile::Error("No authorize sections found");
    }

  std::stringstream dryRunErrors;
  ConfigSection::const_iterator authIt;
  for (authIt = section.begin(); authIt != section.end(); authIt++)
    {
      std::string certfile;
      try
        {
          certfile = authIt->second.get<std::string>("certfile");
        }
      catch (const std::runtime_error& e)
        {
          std::string msg = "No certfile specified";
          if (!isDryRun)
            {
              throw ConfigFile::Error(msg);
            }
          aggregateErrors(dryRunErrors, msg);
          continue;
        }

      path certfilePath = absolute(certfile, path(filename).parent_path());
      NFD_LOG_DEBUG("generated certfile path: " << certfilePath.native());

      std::ifstream in;
      in.open(certfilePath.c_str());
      if (!in.is_open())
        {
          std::string msg = "Unable to open certificate file " + certfilePath.native();
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
          std::string msg = "Malformed certificate file " + certfilePath.native();
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
          std::string msg = "No privileges section found for certificate file " +
            certfile + " (" + id->getPublicKeyName().toUri() + ")";
          if (!isDryRun)
            {
              throw ConfigFile::Error(msg);
            }
          aggregateErrors(dryRunErrors, msg);
          continue;
        }

      if (privileges->begin() == privileges->end())
        {
          NFD_LOG_WARN("No privileges specified for certificate file " << certfile
                       << " (" << id->getPublicKeyName().toUri() << ")");
        }

      ConfigSection::const_iterator privIt;
      for (privIt = privileges->begin(); privIt != privileges->end(); privIt++)
        {
          const std::string& privilegeName = privIt->first;
          if (m_supportedPrivileges.find(privilegeName) != m_supportedPrivileges.end())
            {
              NFD_LOG_INFO("Giving privilege \"" << privilegeName
                           << "\" to identity " << id->getPublicKeyName());
              if (!isDryRun)
                {
                  const std::string regex = "^<localhost><nfd><" + privilegeName + ">";
                  m_validator.addInterestRule(regex, *id);
                }
            }
          else
            {
              // Invalid configuration
              std::string msg = "Invalid privilege \"" + privilegeName + "\" for certificate file " +
                certfile + " (" + id->getPublicKeyName().toUri() + ")";
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
      throw CommandValidator::Error("Duplicated privilege: " + privilege);
    }
  m_supportedPrivileges.insert(privilege);
}

} // namespace nfd
