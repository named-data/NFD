/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#include "tables-config-section.hpp"

#include "common.hpp"
#include "core/logger.hpp"
#include "core/config-file.hpp"

namespace nfd {

NFD_LOG_INIT("TablesConfigSection");

TablesConfigSection::TablesConfigSection(Cs& cs,
                                         Pit& pit,
                                         Fib& fib,
                                         StrategyChoice& strategyChoice,
                                         Measurements& measurements)
  : m_cs(cs)
  // , m_pit(pit)
  // , m_fib(fib)
  // , m_strategyChoice(strategyChoice)
  // , m_measurements(measurements)
{

}

void
TablesConfigSection::onConfig(const ConfigSection& configSection,
                              bool isDryRun,
                              const std::string& filename)
{
  // tables
  // {
  //    cs_max_packets 65536
  // }

  boost::optional<const ConfigSection&> csMaxPacketsNode =
    configSection.get_child_optional("cs_max_packets");

  if (csMaxPacketsNode)
    {
      boost::optional<size_t> valCsMaxPackets =
        configSection.get_optional<size_t>("cs_max_packets");

      if (!valCsMaxPackets)
        {
          throw ConfigFile::Error("Invalid value for option \"cs_max_packets\""
                                  " in \"tables\" section");
        }
      else if (!isDryRun)
        {
          m_cs.setLimit(*valCsMaxPackets);
        }
    }
}

void
TablesConfigSection::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("tables",
                               bind(&TablesConfigSection::onConfig, this, _1, _2, _3));
}

} // namespace nfd
