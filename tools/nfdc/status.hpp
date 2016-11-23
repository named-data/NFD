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

#ifndef NFD_TOOLS_NFDC_STATUS_HPP
#define NFD_TOOLS_NFDC_STATUS_HPP

#include "status-report.hpp"
#include "command-parser.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

struct StatusReportOptions
{
  ReportFormat output = ReportFormat::TEXT;
  bool wantForwarderGeneral = false;
  bool wantChannels = false;
  bool wantFaces = false;
  bool wantFib = false;
  bool wantRib = false;
  bool wantStrategyChoice = false;
};

/** \brief collect a status report and write to stdout
 */
int
reportStatus(ExecuteContext& ctx, const StatusReportOptions& options);

/** \brief registers status commands
 *
 *  Providing the following commands:
 *  \li status report
 *  \li status show
 *  \li face list
 *  \li channel list
 *  \li strategy list
 *  \li fib list
 *  \li route list
 */
void
registerStatusCommands(CommandParser& parser);

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_STATUS_HPP
