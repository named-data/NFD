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

#include "face-module.hpp"
#include "find-face.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
FaceModule::registerCommands(CommandParser& parser)
{
  CommandDefinition defFaceList("face", "list");
  defFaceList
    .setTitle("print face list")
    .addArg("remote", ArgValueType::FACE_URI, Required::NO, Positional::YES)
    .addArg("local", ArgValueType::FACE_URI, Required::NO, Positional::NO)
    .addArg("scheme", ArgValueType::STRING, Required::NO, Positional::NO, "scheme");
  parser.addCommand(defFaceList, &FaceModule::list);
  parser.addAlias("face", "list", "");

  CommandDefinition defFaceShow("face", "show");
  defFaceShow
    .setTitle("show face information")
    .addArg("id", ArgValueType::UNSIGNED, Required::YES, Positional::YES);
  parser.addCommand(defFaceShow, &FaceModule::show);

  CommandDefinition defFaceCreate("face", "create");
  defFaceCreate
    .setTitle("create a face")
    .addArg("remote", ArgValueType::FACE_URI, Required::YES, Positional::YES)
    .addArg("persistency", ArgValueType::FACE_PERSISTENCY, Required::NO, Positional::YES)
    .addArg("local", ArgValueType::FACE_URI, Required::NO, Positional::NO)
    .addArg("reliability", ArgValueType::BOOLEAN, Required::NO, Positional::NO)
    .addArg("congestion-marking", ArgValueType::BOOLEAN, Required::NO, Positional::NO)
    .addArg("congestion-marking-interval", ArgValueType::UNSIGNED, Required::NO, Positional::NO)
    .addArg("default-congestion-threshold", ArgValueType::UNSIGNED, Required::NO, Positional::NO);
  parser.addCommand(defFaceCreate, &FaceModule::create);

  CommandDefinition defFaceDestroy("face", "destroy");
  defFaceDestroy
    .setTitle("destroy a face")
    .addArg("face", ArgValueType::FACE_ID_OR_URI, Required::YES, Positional::YES);
  parser.addCommand(defFaceDestroy, &FaceModule::destroy);
}

void
FaceModule::list(ExecuteContext& ctx)
{
  auto remoteUri = ctx.args.getOptional<FaceUri>("remote");
  auto localUri = ctx.args.getOptional<FaceUri>("local");
  auto uriScheme = ctx.args.getOptional<std::string>("scheme");

  FaceQueryFilter filter;
  if (remoteUri) {
    filter.setRemoteUri(remoteUri->toString());
  }
  if (localUri) {
    filter.setLocalUri(localUri->toString());
  }
  if (uriScheme) {
    filter.setUriScheme(*uriScheme);
  }

  FindFace findFace(ctx);
  FindFace::Code res = findFace.execute(filter, true);

  ctx.exitCode = static_cast<int>(res);
  switch (res) {
    case FindFace::Code::OK:
      for (const FaceStatus& item : findFace.getResults()) {
        formatItemText(ctx.out, item, false);
        ctx.out << '\n';
      }
      break;
    case FindFace::Code::ERROR:
    case FindFace::Code::NOT_FOUND:
    case FindFace::Code::CANONIZE_ERROR:
      ctx.err << findFace.getErrorReason() << '\n';
      break;
    default:
      BOOST_ASSERT_MSG(false, "unexpected FindFace result");
      break;
  }
}

void
FaceModule::show(ExecuteContext& ctx)
{
  uint64_t faceId = ctx.args.get<uint64_t>("id");

  FindFace findFace(ctx);
  FindFace::Code res = findFace.execute(faceId);

  ctx.exitCode = static_cast<int>(res);
  switch (res) {
    case FindFace::Code::OK:
      formatItemText(ctx.out, findFace.getFaceStatus(), true);
      break;
    case FindFace::Code::ERROR:
    case FindFace::Code::NOT_FOUND:
      ctx.err << findFace.getErrorReason() << '\n';
      break;
    default:
      BOOST_ASSERT_MSG(false, "unexpected FindFace result");
      break;
  }
}

/** \brief order persistency in NONE < ON_DEMAND < PERSISTENCY < PERMANENT
 */
static bool
persistencyLessThan(FacePersistency x, FacePersistency y)
{
  switch (x) {
    case FacePersistency::FACE_PERSISTENCY_NONE:
      return y != FacePersistency::FACE_PERSISTENCY_NONE;
    case FacePersistency::FACE_PERSISTENCY_ON_DEMAND:
      return y == FacePersistency::FACE_PERSISTENCY_PERSISTENT ||
             y == FacePersistency::FACE_PERSISTENCY_PERMANENT;
    case FacePersistency::FACE_PERSISTENCY_PERSISTENT:
      return y == FacePersistency::FACE_PERSISTENCY_PERMANENT;
    case FacePersistency::FACE_PERSISTENCY_PERMANENT:
      return false;
  }
  return static_cast<int>(x) < static_cast<int>(y);
}

void
FaceModule::create(ExecuteContext& ctx)
{
  auto remoteUri = ctx.args.get<FaceUri>("remote");
  auto localUri = ctx.args.getOptional<FaceUri>("local");
  auto persistency = ctx.args.get<FacePersistency>("persistency", FacePersistency::FACE_PERSISTENCY_PERSISTENT);
  auto lpReliability = ctx.args.getTribool("reliability");
  auto congestionMarking = ctx.args.getTribool("congestion-marking");
  auto baseCongestionMarkingIntervalMs = ctx.args.getOptional<uint64_t>("congestion-marking-interval");
  auto defaultCongestionThreshold = ctx.args.getOptional<uint64_t>("default-congestion-threshold");

  FaceUri canonicalRemote;
  optional<FaceUri> canonicalLocal;

  auto handleCanonizeError = [&] (const FaceUri& faceUri, const std::string& error) {
    ctx.exitCode = 4;
    ctx.err << "Error when canonizing '" << faceUri << "': " << error << '\n';
  };

  auto printPositiveResult = [&] (const std::string& actionSummary, const ControlParameters& resp) {
    text::ItemAttributes ia;
    ctx.out << actionSummary << ' '
            << ia("id") << resp.getFaceId()
            << ia("local") << resp.getLocalUri()
            << ia("remote") << resp.getUri()
            << ia("persistency") << resp.getFacePersistency();
    printFaceParams(ctx.out, ia, resp);
  };

  auto updateFace = [&printPositiveResult] (ControlParameters respParams, ControlParameters resp) {
    // faces/update response does not have FaceUris, copy from faces/create response
    resp.setLocalUri(respParams.getLocalUri())
        .setUri(respParams.getUri());
    printPositiveResult("face-updated", resp);
  };

  auto handle409 = [&] (const ControlResponse& resp) {
    ControlParameters respParams(resp.getBody());
    if (respParams.getUri() != canonicalRemote.toString()) {
      // we are conflicting with a different face, which is a general error
      return false;
    }

    if (persistencyLessThan(respParams.getFacePersistency(), persistency)) {
      // need to upgrade persistency
      ControlParameters params;
      params.setFaceId(respParams.getFaceId()).setFacePersistency(persistency);
      if (!boost::logic::indeterminate(lpReliability)) {
        params.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, lpReliability);
      }
      ctx.controller.start<ndn::nfd::FaceUpdateCommand>(
          params,
          bind(updateFace, respParams, _1),
          ctx.makeCommandFailureHandler("upgrading face persistency"),
          ctx.makeCommandOptions());
    }
    else if ((!boost::logic::indeterminate(lpReliability) &&
              lpReliability != respParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED)) ||
             (!boost::logic::indeterminate(congestionMarking) &&
              congestionMarking != respParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)) ||
             baseCongestionMarkingIntervalMs ||
             defaultCongestionThreshold) {
      ControlParameters params;
      params.setFaceId(respParams.getFaceId());

      if (!boost::logic::indeterminate(lpReliability) &&
          lpReliability != respParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED)) {
        params.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, lpReliability);
      }
      if (!boost::logic::indeterminate(congestionMarking) &&
          congestionMarking != respParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)) {
        params.setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, congestionMarking);
      }

      if (baseCongestionMarkingIntervalMs) {
        params.setBaseCongestionMarkingInterval(time::milliseconds(*baseCongestionMarkingIntervalMs));
      }

      if (defaultCongestionThreshold) {
        params.setDefaultCongestionThreshold(*defaultCongestionThreshold);
      }

      ctx.controller.start<ndn::nfd::FaceUpdateCommand>(
          params,
          bind(updateFace, respParams, _1),
          ctx.makeCommandFailureHandler("updating face"),
          ctx.makeCommandOptions());
    }
    else {
      // don't do anything
      printPositiveResult("face-exists", respParams);
    }
    return true;
  };

  auto doCreateFace = [&] {
    ControlParameters params;
    params.setUri(canonicalRemote.toString());
    if (canonicalLocal) {
      params.setLocalUri(canonicalLocal->toString());
    }
    params.setFacePersistency(persistency);
    if (!boost::logic::indeterminate(lpReliability)) {
      params.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, lpReliability);
    }
    if (!boost::logic::indeterminate(congestionMarking)) {
      params.setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, congestionMarking);
    }
    if (baseCongestionMarkingIntervalMs) {
      params.setBaseCongestionMarkingInterval(time::milliseconds(*baseCongestionMarkingIntervalMs));
    }
    if (defaultCongestionThreshold) {
      params.setDefaultCongestionThreshold(*defaultCongestionThreshold);
    }

    ctx.controller.start<ndn::nfd::FaceCreateCommand>(
      params,
      bind(printPositiveResult, "face-created", _1),
      [&] (const ControlResponse& resp) {
        if (resp.getCode() == 409 && handle409(resp)) {
          return;
        }
        ctx.makeCommandFailureHandler("creating face")(resp); // invoke general error handler
      },
      ctx.makeCommandOptions());
  };

  remoteUri.canonize(
    [&] (const FaceUri& canonicalUri) {
      canonicalRemote = canonicalUri;
      if (localUri) {
        localUri->canonize(
          [&] (const FaceUri& canonicalUri) {
            canonicalLocal = canonicalUri;
            doCreateFace();
          },
          bind(handleCanonizeError, *localUri, _1),
          ctx.face.getIoService(), ctx.getTimeout());
      }
      else {
        doCreateFace();
      }
    },
    bind(handleCanonizeError, remoteUri, _1),
    ctx.face.getIoService(), ctx.getTimeout());

  ctx.face.processEvents();
}

void
FaceModule::destroy(ExecuteContext& ctx)
{
  const boost::any& faceIdOrUri = ctx.args.at("face");

  FindFace findFace(ctx);
  FindFace::Code res = findFace.execute(faceIdOrUri);

  ctx.exitCode = static_cast<int>(res);
  switch (res) {
    case FindFace::Code::OK:
      break;
    case FindFace::Code::ERROR:
    case FindFace::Code::CANONIZE_ERROR:
    case FindFace::Code::NOT_FOUND:
      ctx.err << findFace.getErrorReason() << '\n';
      return;
    case FindFace::Code::AMBIGUOUS:
      ctx.err << "Multiple faces match specified remote FaceUri. Re-run the command with a FaceId:";
      findFace.printDisambiguation(ctx.err, FindFace::DisambiguationStyle::LOCAL_URI);
      ctx.err << '\n';
      return;
    default:
      BOOST_ASSERT_MSG(false, "unexpected FindFace result");
      return;
  }

  const FaceStatus& face = findFace.getFaceStatus();

  ctx.controller.start<ndn::nfd::FaceDestroyCommand>(
    ControlParameters().setFaceId(face.getFaceId()),
    [&] (const ControlParameters& resp) {
      ctx.out << "face-destroyed ";
      text::ItemAttributes ia;
      ctx.out << ia("id") << face.getFaceId()
              << ia("local") << face.getLocalUri()
              << ia("remote") << face.getRemoteUri()
              << ia("persistency") << face.getFacePersistency();
      printFaceParams(ctx.out, ia, resp);
    },
    ctx.makeCommandFailureHandler("destroying face"),
    ctx.makeCommandOptions());

  ctx.face.processEvents();
}

void
FaceModule::fetchStatus(Controller& controller,
                        const std::function<void()>& onSuccess,
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

  if (!item.hasBaseCongestionMarkingInterval() && !item.hasDefaultCongestionThreshold()) {
    os << "<congestion/>";
  }
  else {
    os << "<congestion>";
    if (item.hasBaseCongestionMarkingInterval()) {
      os << "<baseMarkingInterval>" << xml::formatDuration(item.getBaseCongestionMarkingInterval())
         << "</baseMarkingInterval>";
    }
    if (item.hasDefaultCongestionThreshold()) {
      os << "<defaultThreshold>" << item.getDefaultCongestionThreshold() << "</defaultThreshold>";
    }
    os << "</congestion>";
  }

  if (item.getFlags() == 0) {
    os << "<flags/>";
  }
  else {
    os << "<flags>";
    os << xml::Flag{"localFieldsEnabled", item.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED)};
    os << xml::Flag{"lpReliabilityEnabled", item.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED)};
    os << xml::Flag{"congestionMarkingEnabled", item.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)};
    os << "</flags>";
  }

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
  text::ItemAttributes ia(wantMultiLine, 10);

  os << ia("faceid") << item.getFaceId();
  os << ia("remote") << item.getRemoteUri();
  os << ia("local") << item.getLocalUri();

  if (item.hasExpirationPeriod()) {
    os << ia("expires") << text::formatDuration<time::seconds>(item.getExpirationPeriod());
  }

  if (item.hasBaseCongestionMarkingInterval() || item.hasDefaultCongestionThreshold()) {
    os << ia("congestion") << "{";
    text::Separator congestionSep("", " ");
    if (item.hasBaseCongestionMarkingInterval()) {
      os << congestionSep << "base-marking-interval="
         << text::formatDuration<time::milliseconds>(item.getBaseCongestionMarkingInterval());
    }
    if (item.hasDefaultCongestionThreshold()) {
      os << congestionSep << "default-threshold=" << item.getDefaultCongestionThreshold() << "B";
    }
    os << "}";
  }

  os << ia("counters")
     << "{in={"
     << item.getNInInterests() << "i "
     << item.getNInData() << "d "
     << item.getNInNacks() << "n "
     << item.getNInBytes() << "B} "
     << "out={"
     << item.getNOutInterests() << "i "
     << item.getNOutData() << "d "
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
  if (item.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED)) {
    os << flagSep << "lp-reliability";
  }
  if (item.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)) {
    os << flagSep << "congestion-marking";
  }
  os << '}';

  os << ia.end();
}

void
FaceModule::printFaceParams(std::ostream& os, text::ItemAttributes& ia, const ControlParameters& resp)
{
  os << ia("reliability") << text::OnOff{resp.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED)}
     << ia("congestion-marking") << text::OnOff{resp.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)};
  if (resp.hasBaseCongestionMarkingInterval()) {
    os << ia("congestion-marking-interval")
       << text::formatDuration<time::milliseconds>(resp.getBaseCongestionMarkingInterval());
  }
  if (resp.hasDefaultCongestionThreshold()) {
    os << ia("default-congestion-threshold") << resp.getDefaultCongestionThreshold() << "B";
  }
  os << '\n';
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
