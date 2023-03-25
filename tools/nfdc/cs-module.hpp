/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NFDC_CS_MODULE_HPP
#define NFD_TOOLS_NFDC_CS_MODULE_HPP

#include "module.hpp"
#include "command-parser.hpp"

#include <ndn-cxx/mgmt/nfd/cs-info.hpp>

namespace nfd::tools::nfdc {

using ndn::nfd::CsInfo;

/**
 * \brief Provides access to NFD CS management.
 * \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt
 */
class CsModule : public Module, boost::noncopyable
{
public:
  /** \brief Register 'cs config' command.
   */
  static void
  registerCommands(CommandParser& parser);

  /** \brief The 'cs config' command.
   */
  static void
  config(ExecuteContext& ctx);

  /** \brief The 'cs erase' command.
   */
  static void
  erase(ExecuteContext& ctx);

  void
  fetchStatus(ndn::nfd::Controller& controller,
              const std::function<void()>& onSuccess,
              const ndn::nfd::DatasetFailureCallback& onFailure,
              const CommandOptions& options) override;

  void
  formatStatusXml(std::ostream& os) const override;

  static void
  formatItemXml(std::ostream& os, const CsInfo& item);

  void
  formatStatusText(std::ostream& os) const override;

  static void
  formatItemText(std::ostream& os, const CsInfo& item);

private:
  CsInfo m_status;
};

} // namespace nfd::tools::nfdc

#endif // NFD_TOOLS_NFDC_CS_MODULE_HPP
