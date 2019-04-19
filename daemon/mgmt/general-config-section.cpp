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

#include "general-config-section.hpp"
#include "common/privilege-helper.hpp"

namespace nfd {
namespace general {

static void
onConfig(const ConfigSection& section, bool isDryRun, const std::string&)
{
  // general
  // {
  //   user "ndn-user"
  //   group "ndn-user"
  // }

  std::string user;
  std::string group;

  for (const auto& i : section) {
    if (i.first == "user") {
      try {
        user = i.second.get_value<std::string>("user");
        if (user.empty()) {
          NDN_THROW(ConfigFile::Error("Invalid value for 'user' in section 'general'"));
        }
      }
      catch (const boost::property_tree::ptree_error&) {
        NDN_THROW(ConfigFile::Error("Invalid value for 'user' in section 'general'"));
      }
    }
    else if (i.first == "group") {
      try {
        group = i.second.get_value<std::string>("group");
        if (group.empty()) {
          NDN_THROW(ConfigFile::Error("Invalid value for 'group' in section 'general'"));
        }
      }
      catch (const boost::property_tree::ptree_error&) {
        NDN_THROW(ConfigFile::Error("Invalid value for 'group' in section 'general'"));
      }
    }
  }

  PrivilegeHelper::initialize(user, group);
}

void
setConfigFile(ConfigFile& config)
{
  config.addSectionHandler("general", &onConfig);
}

} // namespace general
} // namespace nfd
