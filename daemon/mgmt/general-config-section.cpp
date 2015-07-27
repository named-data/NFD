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

#include "general-config-section.hpp"

#include "common.hpp"
#include "core/logger.hpp"
#include "core/privilege-helper.hpp"
#include "core/config-file.hpp"

namespace nfd {

namespace general {

NFD_LOG_INIT("GeneralConfigSection");

const ndn::Name
RouterName::getName() const
{
  ndn::Name routerName;

  if (network.empty() || site.empty() || router.empty())
    {
      return routerName;
    }

  routerName = network;
  routerName.append(site);
  routerName.append(ROUTER_MARKER);
  routerName.append(router);

  return routerName;
}

const ndn::PartialName RouterName::ROUTER_MARKER("%C1.Router");

static RouterName&
getRouterNameInstance()
{
  static RouterName routerName;
  return routerName;
}

ndn::PartialName
loadPartialNameFromSection(const ConfigSection& section, const std::string& key)
{
  ndn::PartialName value;

  try
    {
      value = section.get<ndn::PartialName>(key);

      if (value.empty())
        {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for \"router_name." + key + "\""
                                                  " in \"general\" section"));
        }
    }
  catch (const boost::property_tree::ptree_error& error)
    {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for \"router_name." + key + "\""
                                              " in \"general\" section"));
    }

  return value;
}

void
processSectionRouterName(const ConfigSection& section, bool isDryRun)
{
  RouterName& routerName = getRouterNameInstance();

  routerName.network = loadPartialNameFromSection(section, "network");
  routerName.site = loadPartialNameFromSection(section, "site");
  routerName.router = loadPartialNameFromSection(section, "router");
}

static void
onConfig(const ConfigSection& configSection,
         bool isDryRun,
         const std::string& filename)
{
  // general
  // {
  //    ; user "ndn-user"
  //    ; group "ndn-user"
  //
  //    ; router_name
  //    ; {
  //    ;   network ndn
  //    ;   site    edu/site
  //    ;   router  router/name
  //    ; }
  // }

  std::string user;
  std::string group;

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "user")
        {
          try
            {
              user = i->second.get_value<std::string>("user");

              if (user.empty())
                {
                  BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for \"user\""
                                                          " in \"general\" section"));
                }
            }
          catch (const boost::property_tree::ptree_error& error)
            {
              BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for \"user\""
                                                      " in \"general\" section"));
            }
        }
      else if (i->first == "group")
        {
          try
            {
              group = i->second.get_value<std::string>("group");

              if (group.empty())
                {
                  BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for \"group\""
                                                          " in \"general\" section"));
                }
            }
          catch (const boost::property_tree::ptree_error& error)
            {
              BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for \"group\""
                                                      " in \"general\" section"));
            }
        }
    }
  NFD_LOG_TRACE("using user \"" << user << "\" group \"" << group << "\"");

  PrivilegeHelper::initialize(user, group);

  boost::optional<const ConfigSection&> routerNameSection =
    configSection.get_child_optional("router_name");

  if (routerNameSection)
    {
      processSectionRouterName(*routerNameSection, isDryRun);
    }
}

void
setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("general", &onConfig);
}

const RouterName&
getRouterName()
{
  return getRouterNameInstance();
}

} // namespace general

} // namespace nfd
