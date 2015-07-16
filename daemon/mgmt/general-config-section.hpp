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

#ifndef NFD_MGMT_GENERAL_CONFIG_SECTION_HPP
#define NFD_MGMT_GENERAL_CONFIG_SECTION_HPP

#include <ndn-cxx/name.hpp>

namespace nfd {

class ConfigFile;

namespace general {

void
setConfigFile(ConfigFile& configFile);

class RouterName
{
public:
  /**
   * \brief Return the router name constructed from the network, site, and
   *        router variables.
   *
   *        The router name is constructed in the following manner:
   *        /<network>/<site>/<ROUTER_MARKER>/<router>
   *
   * \return The constructed router name if the network, site, and router
   *         configuration options are non-empty; otherwise, an empty ndn::Name.
   */
  const ndn::Name
  getName() const;

public:
  ndn::PartialName network;
  ndn::PartialName site;
  ndn::PartialName router;

  static const ndn::PartialName ROUTER_MARKER;
};

const RouterName&
getRouterName();

} // namespace general

} // namespace nfd

#endif // NFD_MGMT_GENERAL_CONFIG_SECTION_HPP
