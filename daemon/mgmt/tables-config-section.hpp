/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_MGMT_TABLES_CONFIG_SECTION_HPP
#define NFD_MGMT_TABLES_CONFIG_SECTION_HPP

#include "fw/forwarder.hpp"
#include "core/config-file.hpp"

namespace nfd {

/** \brief handles 'tables' config section
 *
 *  This class recognizes a config section that looks like
 *  \code{.unparsed}
 *  tables
 *  {
 *    cs_max_packets 65536
 *    cs_policy priority_fifo
 *    cs_unsolicited_policy drop-all
 *
 *    strategy_choice
 *    {
 *      /               /localhost/nfd/strategy/best-route
 *      /localhost      /localhost/nfd/strategy/multicast
 *      /localhost/nfd  /localhost/nfd/strategy/best-route
 *      /ndn/broadcast  /localhost/nfd/strategy/multicast
 *    }
 *
 *    network_region
 *    {
 *      /example/region1
 *      /example/region2
 *    }
 *  }
 *  \endcode
 *
 *  During a configuration reload,
 *  \li cs_max_packets, cs_policy, and cs_unsolicited_policy are applied;
 *      defaults are used if an option is omitted.
 *  \li strategy_choice entries are inserted, but old entries are not deleted.
 *  \li network_region is applied; it's kept unchanged if the section is omitted.
 *
 *  It's necessary to call \p ensureConfigured() after initial configuration and
 *  configuration reload, so that the correct defaults are applied in case
 *  tables section is omitted.
 */
class TablesConfigSection : noncopyable
{
public:
  explicit
  TablesConfigSection(Forwarder& forwarder);

  void
  setConfigFile(ConfigFile& configFile);

  /** \brief apply default configuration, if tables section was omitted in configuration file
   */
  void
  ensureConfigured();

private:
  void
  processConfig(const ConfigSection& section, bool isDryRun);

  void
  processStrategyChoiceSection(const ConfigSection& section, bool isDryRun);

  void
  processNetworkRegionSection(const ConfigSection& section, bool isDryRun);

private:
  static const size_t DEFAULT_CS_MAX_PACKETS;

  Forwarder& m_forwarder;

  bool m_isConfigured;
};

} // namespace nfd

#endif // NFD_MGMT_TABLES_CONFIG_SECTION_HPP
