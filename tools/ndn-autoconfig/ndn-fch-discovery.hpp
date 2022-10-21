/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_NDN_FCH_DISCOVERY_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_NDN_FCH_DISCOVERY_HPP

#include "stage.hpp"

namespace ndn::autoconfig {

/**
 * @brief Discover NDN hub using NDN-FCH protocol.
 *
 * @see https://github.com/named-data/ndn-fch/blob/master/README.md
 */
class NdnFchDiscovery : public Stage
{
public:
  /**
   * @brief Create stage to discover NDN hub using NDN-FCH protocol.
   */
  explicit
  NdnFchDiscovery(const std::string& url);

  const std::string&
  getName() const override
  {
    static const std::string STAGE_NAME("NDN-FCH");
    return STAGE_NAME;
  }

private:
  void
  doStart() override;

private:
  std::string m_url;
};

} // namespace ndn::autoconfig

#endif // NFD_TOOLS_NDN_AUTOCONFIG_NDN_FCH_DISCOVERY_HPP
