/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "face-module.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
FaceModule::registerCommands(CommandParser& parser)
{
  CommandDefinition defFaceShow("face", "show");
  defFaceShow
    .setTitle("show face information")
    .addArg("id", ArgValueType::UNSIGNED, Required::YES, Positional::YES);
  parser.addCommand(defFaceShow, &FaceModule::show);
}

void
FaceModule::show(ExecuteContext& ctx)
{
  uint64_t faceId = ctx.args.get<uint64_t>("id");

  ndn::nfd::FaceQueryFilter filter;
  filter.setFaceId(faceId);
  ctx.controller.fetch<ndn::nfd::FaceQueryDataset>(
    filter,
    [faceId, &ctx] (const std::vector<FaceStatus>& result) {
      if (result.size() != 1) {
        ctx.exitCode = 3;
        ctx.err << "Face " << faceId << " not found.\n";
        return;
      }
      formatItemText(ctx.out, result.front(), true);
    },
    ctx.makeDatasetFailureHandler("face information"));

  ctx.face.processEvents();
}

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

  if (item.getFlags() == 0) {
    os << "<flags/>";
  }
  else {
    os << "<flags>";
    if (item.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED)) {
      os << "<localFieldsEnabled/>";
    }
    os << "</flags>";
  }

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
    os << "  ";
    formatItemText(os, item, false);
    os << '\n';
  }
}

void
FaceModule::formatItemText(std::ostream& os, const FaceStatus& item, bool wantMultiLine)
{
  text::ItemAttributes ia(wantMultiLine, 8);

  os << ia("faceid") << item.getFaceId();
  os << ia("remote") << item.getRemoteUri();
  os << ia("local") << item.getLocalUri();

  if (item.hasExpirationPeriod()) {
    os << ia("expires") << text::formatDuration(item.getExpirationPeriod());
  }

  os << ia("counters")
     << "{in={"
     << item.getNInInterests() << "i "
     << item.getNInDatas() << "d "
     << item.getNInNacks() << "n "
     << item.getNInBytes() << "B} "
     << "out={"
     << item.getNOutInterests() << "i "
     << item.getNOutDatas() << "d "
     << item.getNOutNacks() << "n "
     << item.getNOutBytes() << "B}}";

  os << ia("flags") << '{';
  text::Separator flagSep("", " ");
  os << flagSep << item.getFaceScope();
  os << flagSep << item.getFacePersistency();
  os << flagSep << item.getLinkType();
  if (item.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED)) {
    os << flagSep << "local-fields";
  }
  os << '}';

  os << ia.end();
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
