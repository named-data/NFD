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

/** \todo
 *  #2542 plans to merge nfd-status with nfdc.
 *  FaceModule class should be changed as follows:
 *  (1) move into ndn::nfd::nfdc namespace
 *  (2) assuming command syntax is similar to Windows NT \p netsh ,
 *      this class can handle command argument parsing as soon as
 *      'face' sub-command is detected
 *  (3) introduce methods to create and destroy faces, and update face attributes
 *
 *  \todo
 *  #3444 aims at improving output texts of nfdc.
 *  Assuming it's worked with or after #2542:
 *  (1) introduce an \p style parameter on formatItemText method to specify desired text style,
 *      such as human friendly style vs. porcelain style for script consumption
 *  (2) upon successful command execute, convert the command result into FaceStatus type,
 *      and use formatItemText to render the result, so that output from status retrieval
 *      and command execution have consistent styles
 *  }
 */

#include "face-module.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
FaceModule::fetchStatus(Controller& controller,
                        const function<void()>& onSuccess,
                        const Controller::DatasetFailCallback& onFailure,
                        const CommandOptions& options)
{
  controller.fetch<ndn::nfd::FaceDataset>(
    [this, onSuccess] (const std::vector<FaceStatus>& result) {
      m_status = result;
      onSuccess();
    },
    onFailure, options);
}

void
FaceModule::formatStatusXml(std::ostream& os) const
{
  os << "<faces>";
  for (const FaceStatus& item : m_status) {
    this->formatItemXml(os, item);
  }
  os << "</faces>";
}

void
FaceModule::formatItemXml(std::ostream& os, const FaceStatus& item) const
{
  os << "<face>";

  os << "<faceId>" << item.getFaceId() << "</faceId>";
  os << "<remoteUri>" << xml::Text{item.getRemoteUri()} << "</remoteUri>";
  os << "<localUri>" << xml::Text{item.getLocalUri()} << "</localUri>";

  if (item.hasExpirationPeriod()) {
    os << "<expirationPeriod>" << xml::formatDuration(item.getExpirationPeriod())
       << "</expirationPeriod>";
  }
  os << "<faceScope>" << item.getFaceScope() << "</faceScope>";
  os << "<facePersistency>" << item.getFacePersistency() << "</facePersistency>";
  os << "<linkType>" << item.getLinkType() << "</linkType>";

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

  os << "<byteCounters>";
  os << "<incomingBytes>" << item.getNInBytes() << "</incomingBytes>";
  os << "<outgoingBytes>" << item.getNOutBytes() << "</outgoingBytes>";
  os << "</byteCounters>";

  os << "</face>";
}

void
FaceModule::formatStatusText(std::ostream& os) const
{
  os << "Faces:\n";
  for (const FaceStatus& item : m_status) {
    this->formatItemText(os, item);
  }
}

void
FaceModule::formatItemText(std::ostream& os, const FaceStatus& item) const
{
  os << "  faceid=" << item.getFaceId();
  os << " remote=" << item.getRemoteUri();
  os << " local=" << item.getLocalUri();

  if (item.hasExpirationPeriod()) {
    os << " expires=" << text::formatDuration(item.getExpirationPeriod());
  }

  os << " counters={in={"
     << item.getNInInterests() << "i "
     << item.getNInDatas() << "d "
     << item.getNInNacks() << "n "
     << item.getNInBytes() << "B} ";
  os << "out={"
     << item.getNOutInterests() << "i "
     << item.getNOutDatas() << "d "
     << item.getNOutNacks() << "n "
     << item.getNOutBytes() << "B}}";

  os << " " << item.getFaceScope();
  os << " " << item.getFacePersistency();
  os << " " << item.getLinkType();
  os << "\n";
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
