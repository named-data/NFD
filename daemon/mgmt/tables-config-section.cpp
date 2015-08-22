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

const size_t TablesConfigSection::DEFAULT_CS_MAX_PACKETS = 65536;

TablesConfigSection::TablesConfigSection(Cs& cs,
                                         Pit& pit,
                                         Fib& fib,
                                         StrategyChoice& strategyChoice,
                                         Measurements& measurements)
  : m_cs(cs)
  // , m_pit(pit)
  // , m_fib(fib)
  , m_strategyChoice(strategyChoice)
  // , m_measurements(measurements)
  , m_areTablesConfigured(false)
{

}

void
TablesConfigSection::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("tables",
                               bind(&TablesConfigSection::onConfig, this, _1, _2, _3));
}


void
TablesConfigSection::ensureTablesAreConfigured()
{
  if (m_areTablesConfigured)
    {
      return;
    }

  NFD_LOG_INFO("Setting CS max packets to " << DEFAULT_CS_MAX_PACKETS);
  m_cs.setLimit(DEFAULT_CS_MAX_PACKETS);

  m_areTablesConfigured = true;
}

void
TablesConfigSection::onConfig(const ConfigSection& configSection,
                              bool isDryRun,
                              const std::string& filename)
{
  // tables
  // {
  //    cs_max_packets 65536
  //
  //    strategy_choice
  //    {
  //       /               /localhost/nfd/strategy/best-route
  //       /localhost      /localhost/nfd/strategy/multicast
  //       /localhost/nfd  /localhost/nfd/strategy/best-route
  //       /ndn/broadcast  /localhost/nfd/strategy/multicast
  //    }
  // }

  size_t nCsMaxPackets = DEFAULT_CS_MAX_PACKETS;

  boost::optional<const ConfigSection&> csMaxPacketsNode =
    configSection.get_child_optional("cs_max_packets");

  if (csMaxPacketsNode)
    {
      boost::optional<size_t> valCsMaxPackets =
        configSection.get_optional<size_t>("cs_max_packets");

      if (!valCsMaxPackets)
        {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for option \"cs_max_packets\""
                                                  " in \"tables\" section"));
        }

      nCsMaxPackets = *valCsMaxPackets;
    }

  boost::optional<const ConfigSection&> strategyChoiceSection =
    configSection.get_child_optional("strategy_choice");

  if (strategyChoiceSection)
    {
      processSectionStrategyChoice(*strategyChoiceSection, isDryRun);
    }

  if (!isDryRun)
    {
      NFD_LOG_INFO("Setting CS max packets to " << nCsMaxPackets);

      m_cs.setLimit(nCsMaxPackets);
      m_areTablesConfigured = true;
    }
}

void
TablesConfigSection::processSectionStrategyChoice(const ConfigSection& configSection,
                                                  bool isDryRun)
{
  // strategy_choice
  // {
  //   /               /localhost/nfd/strategy/best-route
  //   /localhost      /localhost/nfd/strategy/multicast
  //   /localhost/nfd  /localhost/nfd/strategy/best-route
  //   /ndn/broadcast  /localhost/nfd/strategy/multicast
  // }

  std::map<Name, Name> choices;

  for (const auto& prefixAndStrategy : configSection)
    {
      const Name prefix(prefixAndStrategy.first);
      if (choices.find(prefix) != choices.end())
        {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("Duplicate strategy choice for prefix \"" +
                                                  prefix.toUri() + "\" in \"strategy_choice\" "
                                                  "section"));
        }

      const std::string strategyString(prefixAndStrategy.second.get_value<std::string>());
      if (strategyString.empty())
        {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid strategy choice \"\" for prefix \"" +
                                                  prefix.toUri() + "\" in \"strategy_choice\" "
                                                  "section"));
        }

      const Name strategyName(strategyString);
      if (!m_strategyChoice.hasStrategy(strategyName))
        {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid strategy choice \"" +
                                                  strategyName.toUri() + "\" for prefix \"" +
                                                  prefix.toUri() + "\" in \"strategy_choice\" "
                                                  "section"));
        }

      choices[prefix] = strategyName;
    }


  for (const auto& prefixAndStrategy : choices)
    {
      if (!isDryRun && !m_strategyChoice.insert(prefixAndStrategy.first, prefixAndStrategy.second))
        {
          BOOST_THROW_EXCEPTION(ConfigFile::Error("Failed to set strategy \"" +
                                                  prefixAndStrategy.second.toUri() + "\" for "
                                                  "prefix \"" + prefixAndStrategy.first.toUri() +
                                                  "\" in \"strategy_choicev\""));
        }
    }
}



} // namespace nfd
