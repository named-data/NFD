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

#include "fib-module.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
FibModule::fetchStatus(Controller& controller,
                       const std::function<void()>& onSuccess,
                       const Controller::DatasetFailCallback& onFailure,
                       const CommandOptions& options)
{
  controller.fetch<ndn::nfd::FibDataset>(
    [this, onSuccess] (const std::vector<FibEntry>& result) {
      m_status = result;
      onSuccess();
    },
    onFailure, options);
}

void
FibModule::formatStatusXml(std::ostream& os) const
{
  os << "<fib>";
  for (const FibEntry& item : m_status) {
    this->formatItemXml(os, item);
  }
  os << "</fib>";
}

void
FibModule::formatItemXml(std::ostream& os, const FibEntry& item) const
{
  os << "<fibEntry>";

  os << "<prefix>" << xml::Text{item.getPrefix().toUri()} << "</prefix>";

  os << "<nextHops>";
  for (const NextHopRecord& nh : item.getNextHopRecords()) {
    os << "<nextHop>"
       << "<faceId>" << nh.getFaceId() << "</faceId>"
       << "<cost>" << nh.getCost() << "</cost>"
       << "</nextHop>";
  }
  os << "</nextHops>";

  os << "</fibEntry>";
}

void
FibModule::formatStatusText(std::ostream& os) const
{
  os << "FIB:\n";
  for (const FibEntry& item : m_status) {
    this->formatItemText(os, item);
  }
}

void
FibModule::formatItemText(std::ostream& os, const FibEntry& item) const
{
  os << "  " << item.getPrefix() << " nexthops={";

  text::Separator sep(", ");
  for (const NextHopRecord& nh : item.getNextHopRecords()) {
    os << sep
       << "faceid=" << nh.getFaceId()
       << " (cost=" << nh.getCost() << ")";
  }

  os << "}";
  os << "\n";
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
