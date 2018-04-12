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

#include "forwarder-general-module.hpp"
#include "format-helpers.hpp"

#include <ndn-cxx/util/indented-stream.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

void
ForwarderGeneralModule::fetchStatus(Controller& controller,
                                    const std::function<void()>& onSuccess,
                                    const Controller::DatasetFailCallback& onFailure,
                                    const CommandOptions& options)
{
  controller.fetch<ndn::nfd::ForwarderGeneralStatusDataset>(
    [this, onSuccess] (const ForwarderStatus& result) {
      m_status = result;
      onSuccess();
    },
    onFailure, options);
}

static time::system_clock::Duration
calculateUptime(const ForwarderStatus& status)
{
  return status.getCurrentTimestamp() - status.getStartTimestamp();
}

void
ForwarderGeneralModule::formatStatusXml(std::ostream& os) const
{
  formatItemXml(os, m_status);
}

void
ForwarderGeneralModule::formatItemXml(std::ostream& os, const ForwarderStatus& item)
{
  os << "<generalStatus>";

  os << "<version>" << xml::Text{item.getNfdVersion()} << "</version>";
  os << "<startTime>" << xml::formatTimestamp(item.getStartTimestamp()) << "</startTime>";
  os << "<currentTime>" << xml::formatTimestamp(item.getCurrentTimestamp()) << "</currentTime>";
  os << "<uptime>" << xml::formatDuration(time::duration_cast<time::seconds>(calculateUptime(item)))
     << "</uptime>";

  os << "<nNameTreeEntries>" << item.getNNameTreeEntries() << "</nNameTreeEntries>";
  os << "<nFibEntries>" << item.getNFibEntries() << "</nFibEntries>";
  os << "<nPitEntries>" << item.getNPitEntries() << "</nPitEntries>";
  os << "<nMeasurementsEntries>" << item.getNMeasurementsEntries() << "</nMeasurementsEntries>";
  os << "<nCsEntries>" << item.getNCsEntries() << "</nCsEntries>";

  os << "<packetCounters>";
  os << "<incomingPackets>"
     << "<nInterests>" << item.getNInInterests() << "</nInterests>"
     << "<nData>" << item.getNInData() << "</nData>"
     << "<nNacks>" << item.getNInNacks() << "</nNacks>"
     << "</incomingPackets>";
  os << "<outgoingPackets>"
     << "<nInterests>" << item.getNOutInterests() << "</nInterests>"
     << "<nData>" << item.getNOutData() << "</nData>"
     << "<nNacks>" << item.getNOutNacks() << "</nNacks>"
     << "</outgoingPackets>";
  os << "</packetCounters>";

  os << "</generalStatus>";
}

void
ForwarderGeneralModule::formatStatusText(std::ostream& os) const
{
  os << "General NFD status:\n";
  ndn::util::IndentedStream indented(os, "  ");
  formatItemText(indented, m_status);
}

void
ForwarderGeneralModule::formatItemText(std::ostream& os, const ForwarderStatus& item)
{
  text::ItemAttributes ia(true, 20);

  os << ia("version") << item.getNfdVersion()
     << ia("startTime") << text::formatTimestamp(item.getStartTimestamp())
     << ia("currentTime") << text::formatTimestamp(item.getCurrentTimestamp())
     << ia("uptime") << text::formatDuration<time::seconds>(calculateUptime(item), true);

  os << ia("nNameTreeEntries") << item.getNNameTreeEntries()
     << ia("nFibEntries") << item.getNFibEntries()
     << ia("nPitEntries") << item.getNPitEntries()
     << ia("nMeasurementsEntries") << item.getNMeasurementsEntries()
     << ia("nCsEntries") << item.getNCsEntries();

  os << ia("nInInterests") << item.getNInInterests()
     << ia("nOutInterests") << item.getNOutInterests()
     << ia("nInData") << item.getNInData()
     << ia("nOutData") << item.getNOutData()
     << ia("nInNacks") << item.getNInNacks()
     << ia("nOutNacks") << item.getNOutNacks();

  os << ia.end();
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
