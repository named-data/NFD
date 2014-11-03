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

#ifndef NFD_MGMT_TABLES_CONFIG_SECTION_HPP
#define NFD_MGMT_TABLES_CONFIG_SECTION_HPP

#include "table/fib.hpp"
#include "table/pit.hpp"
#include "table/cs.hpp"
#include "table/measurements.hpp"
#include "table/strategy-choice.hpp"

#include "core/config-file.hpp"

namespace nfd {

class TablesConfigSection
{
public:
  TablesConfigSection(Cs& cs,
                      Pit& pit,
                      Fib& fib,
                      StrategyChoice& strategyChoice,
                      Measurements& measurements);

  void
  setConfigFile(ConfigFile& configFile);

  void
  ensureTablesAreConfigured();

private:

  void
  onConfig(const ConfigSection& configSection,
           bool isDryRun,
           const std::string& filename);

  void
  processSectionStrategyChoice(const ConfigSection& configSection,
                               bool isDryRun);

private:
  Cs& m_cs;
  // Pit& m_pit;
  // Fib& m_fib;
  StrategyChoice& m_strategyChoice;
  // Measurements& m_measurements;

  bool m_areTablesConfigured;

private:

  static const size_t DEFAULT_CS_MAX_PACKETS;
};

} // namespace nfd

#endif // NFD_MGMT_TABLES_CONFIG_SECTION_HPP
