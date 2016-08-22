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

#include "forwarder-general-module.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

ForwarderGeneralModule::ForwarderGeneralModule()
  : m_nfdIdCollector(nullptr)
{
}

void
ForwarderGeneralModule::fetchStatus(Controller& controller,
                                    const function<void()>& onSuccess,
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

const Name&
ForwarderGeneralModule::getNfdId() const
{
  if (m_nfdIdCollector != nullptr && m_nfdIdCollector->hasNfdId()) {
    return m_nfdIdCollector->getNfdId();
  }

  static Name unavailable("/nfdId-unavailable");
  return unavailable;
}

void
ForwarderGeneralModule::formatStatusXml(std::ostream& os) const
{
  this->formatItemXml(os, m_status, this->getNfdId());
}

void
ForwarderGeneralModule::formatItemXml(std::ostream& os, const ForwarderStatus& item,
                                      const Name& nfdId) const
{
  os << "<generalStatus>";

  os << "<nfdId>" << nfdId << "</nfdId>";
  os << "<version>" << xml::Text{item.getNfdVersion()} << "</version>";
  os << "<startTime>" << xml::formatTimestamp(item.getStartTimestamp()) << "</startTime>";
  os << "<currentTime>" << xml::formatTimestamp(item.getCurrentTimestamp()) << "</currentTime>";
  os << "<uptime>" << xml::formatDuration(calculateUptime(item)) << "</uptime>";

  os << "<nNameTreeEntries>" << item.getNNameTreeEntries() << "</nNameTreeEntries>";
  os << "<nFibEntries>" << item.getNFibEntries() << "</nFibEntries>";
  os << "<nPitEntries>" << item.getNPitEntries() << "</nPitEntries>";
  os << "<nMeasurementsEntries>" << item.getNMeasurementsEntries() << "</nMeasurementsEntries>";
  os << "<nCsEntries>" << item.getNCsEntries() << "</nCsEntries>";

  os << "<packetCounters>";
  os << "<incomingPackets>"
     << "<nInterests>" << item.getNInInterests() << "</nInterests>"
     << "<nDatas>" << item.getNInDatas() << "</nDatas>"
     << "<nNacks>" << item.getNInNacks() << "</nNacks>"
     << "</incomingPackets>";
  os << "<outgoingPackets>"
     << "<nInterests>" << item.getNOutInterests() << "</nInterests>"
     << "<nDatas>" << item.getNOutDatas() << "</nDatas>"
     << "<nNacks>" << item.getNOutNacks() << "</nNacks>"
     << "</outgoingPackets>";
  os << "</packetCounters>";

  os << "</generalStatus>";
}

void
ForwarderGeneralModule::formatStatusText(std::ostream& os) const
{
  os << "General NFD status:\n";
  this->formatItemText(os, m_status, this->getNfdId());
}

void
ForwarderGeneralModule::formatItemText(std::ostream& os, const ForwarderStatus& item,
                                       const Name& nfdId) const
{
  os << "                 nfdId=" << nfdId << "\n";
  os << "               version=" << item.getNfdVersion() << "\n";
  os << "             startTime=" << text::formatTimestamp(item.getStartTimestamp()) << "\n";
  os << "           currentTime=" << text::formatTimestamp(item.getCurrentTimestamp()) << "\n";
  os << "                uptime=" << text::formatDuration(calculateUptime(item), true) << "\n";

  os << "      nNameTreeEntries=" << item.getNNameTreeEntries() << "\n";
  os << "           nFibEntries=" << item.getNFibEntries() << "\n";
  os << "           nPitEntries=" << item.getNPitEntries() << "\n";
  os << "  nMeasurementsEntries=" << item.getNMeasurementsEntries() << "\n";
  os << "            nCsEntries=" << item.getNCsEntries() << "\n";

  os << "          nInInterests=" << item.getNInInterests() << "\n";
  os << "         nOutInterests=" << item.getNOutInterests() << "\n";
  os << "              nInDatas=" << item.getNInDatas() << "\n";
  os << "             nOutDatas=" << item.getNOutDatas() << "\n";
  os << "              nInNacks=" << item.getNInNacks() << "\n";
  os << "             nOutNacks=" << item.getNOutNacks() << "\n";
}

NfdIdCollector::NfdIdCollector(unique_ptr<ndn::Validator> inner)
  : m_inner(std::move(inner))
  , m_hasNfdId(false)
{
  BOOST_ASSERT(m_inner != nullptr);
}

const Name&
NfdIdCollector::getNfdId() const
{
  if (!m_hasNfdId) {
    BOOST_THROW_EXCEPTION(std::runtime_error("NfdId is unavailable"));
  }

  return m_nfdId;
}

void
NfdIdCollector::checkPolicy(const Data& data, int nSteps,
                            const ndn::OnDataValidated& accept,
                            const ndn::OnDataValidationFailed& reject,
                            std::vector<shared_ptr<ndn::ValidationRequest>>& nextSteps)
{
  ndn::OnDataValidated accepted = [this, accept] (const shared_ptr<const Data>& data) {
    accept(data); // pass the Data to Validator user's validated callback

    if (m_hasNfdId) {
      return;
    }

    const ndn::Signature& sig = data->getSignature();
    if (!sig.hasKeyLocator()) {
      return;
    }

    const ndn::KeyLocator& kl = sig.getKeyLocator();
    if (kl.getType() != ndn::KeyLocator::KeyLocator_Name) {
      return;
    }

    m_nfdId = kl.getName();
    m_hasNfdId = true;
  };

  BOOST_ASSERT(nSteps == 0);
  m_inner->validate(data, accepted, reject);
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
