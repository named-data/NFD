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

#include "command-validator.hpp"
#include "core/logger.hpp"

#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/security/identity-certificate.hpp>

#include <boost/filesystem.hpp>
#include <fstream>

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

  m_validator.reset();

  if (section.begin() == section.end())
    {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("No authorize sections found"));
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
              BOOST_THROW_EXCEPTION(ConfigFile::Error(msg));
            }
          aggregateErrors(dryRunErrors, msg);
          continue;
        }

      shared_ptr<ndn::IdentityCertificate> id;

      if (certfile != "any")
        {
          path certfilePath = absolute(certfile, path(filename).parent_path());
          NFD_LOG_DEBUG("generated certfile path: " << certfilePath.native());

          std::ifstream in;
          in.open(certfilePath.c_str());
          if (!in.is_open())
            {
              std::string msg = "Unable to open certificate file " + certfilePath.native();
              if (!isDryRun)
                {
                  BOOST_THROW_EXCEPTION(ConfigFile::Error(msg));
                }
              aggregateErrors(dryRunErrors, msg);
              continue;
            }

          try
            {
              id = ndn::io::load<ndn::IdentityCertificate>(in);
            }
          catch (const std::runtime_error& error)
            {
              // do nothing
            }

          if (!static_cast<bool>(id)) {
            std::string msg = "Malformed certificate file " + certfilePath.native();
            if (!isDryRun)
              {
                BOOST_THROW_EXCEPTION(ConfigFile::Error(msg));
              }
            aggregateErrors(dryRunErrors, msg);
            continue;
          }

          in.close();
        }

      std::string keyNameForLogging;
      if (static_cast<bool>(id))
        keyNameForLogging = id->getPublicKeyName().toUri();
      else
        {
          keyNameForLogging = "wildcard";
          NFD_LOG_WARN("Wildcard identity is intended for demo purpose only and " <<
                       "SHOULD NOT be used in production environment");
        }
      const ConfigSection* privileges = 0;
      try
        {
          privileges = &authIt->second.get_child("privileges");
        }
      catch (const std::runtime_error& error)
        {
          std::string msg = "No privileges section found for certificate file " +
            certfile + " (" + keyNameForLogging + ")";
          if (!isDryRun)
            {
              BOOST_THROW_EXCEPTION(ConfigFile::Error(msg));
            }
          aggregateErrors(dryRunErrors, msg);
          continue;
        }

      if (privileges->begin() == privileges->end())
        {
          NFD_LOG_WARN("No privileges specified for certificate file " << certfile
                       << " (" << keyNameForLogging << ")");
        }

      ConfigSection::const_iterator privIt;
      for (privIt = privileges->begin(); privIt != privileges->end(); privIt++)
        {
          const std::string& privilegeName = privIt->first;
          if (m_supportedPrivileges.find(privilegeName) != m_supportedPrivileges.end())
            {
              NFD_LOG_INFO("Giving privilege \"" << privilegeName
                           << "\" to identity " << keyNameForLogging);
              if (!isDryRun)
                {
                  const std::string regex = "^<localhost><nfd><" + privilegeName + ">";
                  if (static_cast<bool>(id))
                    m_validator.addInterestRule(regex, *id);
                  else
                    m_validator.addInterestBypassRule(regex);
                }
            }
          else
            {
              // Invalid configuration
              std::string msg = "Invalid privilege \"" + privilegeName +
                "\" for certificate file " + certfile + " (" + keyNameForLogging + ")";
              if (!isDryRun)
                {
                  BOOST_THROW_EXCEPTION(ConfigFile::Error(msg));
                }
              aggregateErrors(dryRunErrors, msg);
            }
        }
    }

  if (!dryRunErrors.str().empty())
    {
      BOOST_THROW_EXCEPTION(ConfigFile::Error(dryRunErrors.str()));
    }
}

void
CommandValidator::addSupportedPrivilege(const std::string& privilege)
{
  if (m_supportedPrivileges.find(privilege) != m_supportedPrivileges.end())
    {
      BOOST_THROW_EXCEPTION(CommandValidator::Error("Duplicated privilege: " + privilege));
    }
  m_supportedPrivileges.insert(privilege);
}

} // namespace nfd
