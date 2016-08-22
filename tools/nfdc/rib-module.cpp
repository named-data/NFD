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

#include "rib-module.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
RibModule::fetchStatus(Controller& controller,
                       const function<void()>& onSuccess,
                       const Controller::DatasetFailCallback& onFailure,
                       const CommandOptions& options)
{
  controller.fetch<ndn::nfd::RibDataset>(
    [this, onSuccess] (const std::vector<RibEntry>& result) {
      m_status = result;
      onSuccess();
    },
    onFailure, options);
}

void
RibModule::formatStatusXml(std::ostream& os) const
{
  os << "<rib>";
  for (const RibEntry& item : m_status) {
    this->formatItemXml(os, item);
  }
  os << "</rib>";
}

void
RibModule::formatItemXml(std::ostream& os, const RibEntry& item) const
{
  os << "<ribEntry>";

  os << "<prefix>" << xml::Text{item.getName().toUri()} << "</prefix>";

  os << "<routes>";
  for (const Route& route : item.getRoutes()) {
    os << "<route>"
       << "<faceId>" << route.getFaceId() << "</faceId>"
       << "<origin>" << route.getOrigin() << "</origin>"
       << "<cost>" << route.getCost() << "</cost>";
    if (route.getFlags() == ndn::nfd::ROUTE_FLAGS_NONE) {
       os << "<flags/>";
    }
    else {
       os << "<flags>";
      if (route.isChildInherit()) {
        os << "<childInherit/>";
      }
      if (route.isRibCapture()) {
        os << "<ribCapture/>";
      }
      os << "</flags>";
    }
    if (!route.hasInfiniteExpirationPeriod()) {
      os << "<expirationPeriod>"
         << xml::formatDuration(route.getExpirationPeriod())
         << "</expirationPeriod>";
    }
    os << "</route>";
  }
  os << "</routes>";

  os << "</ribEntry>";
}

void
RibModule::formatStatusText(std::ostream& os) const
{
  os << "RIB:\n";
  for (const RibEntry& item : m_status) {
    this->formatItemText(os, item);
  }
}

void
RibModule::formatItemText(std::ostream& os, const RibEntry& item) const
{
  os << "  " << item.getName() << " route={";

  text::Separator sep(", ");
  for (const Route& route : item.getRoutes()) {
    os << sep
       << "faceid=" << route.getFaceId()
       << " (origin=" << route.getOrigin()
       << " cost=" << route.getCost();
    if (!route.hasInfiniteExpirationPeriod()) {
      os << " expires=" << text::formatDuration(route.getExpirationPeriod());
    }
    if (route.isChildInherit()) {
      os << " ChildInherit";
    }
    if (route.isRibCapture()) {
      os << " RibCapture";
    }
    os << ")";
  }

  os << "}";
  os << "\n";
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
