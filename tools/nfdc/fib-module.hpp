/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NFDC_FIB_MODULE_HPP
#define NFD_TOOLS_NFDC_FIB_MODULE_HPP

#include "module.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

using ndn::nfd::FibEntry;
using ndn::nfd::NextHopRecord;

/** \brief provides access to NFD FIB management
 *  \sa https://redmine.named-data.net/projects/nfd/wiki/FibMgmt
 */
class FibModule : public Module, noncopyable
{
public:
  void
  fetchStatus(Controller& controller,
              const std::function<void()>& onSuccess,
              const Controller::DatasetFailCallback& onFailure,
              const CommandOptions& options) override;

  void
  formatStatusXml(std::ostream& os) const override;

  /** \brief format a single status item as XML
   *  \param os output stream
   *  \param item status item
   */
  void
  formatItemXml(std::ostream& os, const FibEntry& item) const;

  void
  formatStatusText(std::ostream& os) const override;

  /** \brief format a single status item as text
   *  \param os output stream
   *  \param item status item
   */
  void
  formatItemText(std::ostream& os, const FibEntry& item) const;

private:
  std::vector<FibEntry> m_status;
};

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_FIB_MODULE_HPP
