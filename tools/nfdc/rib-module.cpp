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

#include "rib-module.hpp"
#include "find-face.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
RibModule::registerCommands(CommandParser& parser)
{
  CommandDefinition defRouteAdd("route", "add");
  defRouteAdd
    .setTitle("add a route")
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES)
    .addArg("nexthop", ArgValueType::FACE_ID_OR_URI, Required::YES, Positional::YES)
    .addArg("origin", ArgValueType::UNSIGNED, Required::NO, Positional::NO)
    .addArg("cost", ArgValueType::UNSIGNED, Required::NO, Positional::NO)
    .addArg("no-inherit", ArgValueType::NONE, Required::NO, Positional::NO)
    .addArg("capture", ArgValueType::NONE, Required::NO, Positional::NO)
    .addArg("expires", ArgValueType::UNSIGNED, Required::NO, Positional::NO);
  parser.addCommand(defRouteAdd, &RibModule::add);
}

void
RibModule::add(ExecuteContext& ctx)
{
  auto prefix = ctx.args.get<Name>("prefix");
  const boost::any& nexthop = ctx.args.at("nexthop");
  auto origin = ctx.args.get<uint64_t>("origin", ndn::nfd::ROUTE_ORIGIN_STATIC);
  auto cost = ctx.args.get<uint64_t>("cost", 0);
  bool wantChildInherit = !ctx.args.get<bool>("no-inherit", false);
  bool wantCapture = ctx.args.get<bool>("capture", false);
  auto expiresMillis = ctx.args.getOptional<uint64_t>("expires");

  FindFace findFace(ctx);
  FindFace::Code res = findFace.execute(nexthop);

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

  ControlParameters registerParams;
  registerParams
    .setName(prefix)
    .setFaceId(findFace.getFaceId())
    .setOrigin(origin)
    .setCost(cost)
    .setFlags((wantChildInherit ? ndn::nfd::ROUTE_FLAG_CHILD_INHERIT : 0) |
              (wantCapture ? ndn::nfd::ROUTE_FLAG_CAPTURE : 0));
  if (expiresMillis) {
    registerParams.setExpirationPeriod(time::milliseconds(*expiresMillis));
  }

  ctx.controller.start<ndn::nfd::RibRegisterCommand>(
    registerParams,
    [&] (const ControlParameters& resp) {
      ctx.out << "route-add-accepted ";
      text::ItemAttributes ia;
      ctx.out << ia("prefix") << resp.getName()
              << ia("nexthop") << resp.getFaceId()
              << ia("origin") << resp.getOrigin()
              << ia("cost") << resp.getCost()
              << ia("flags") << static_cast<ndn::nfd::RouteFlags>(resp.getFlags());
      if (resp.hasExpirationPeriod()) {
        ctx.out << ia("expires") << resp.getExpirationPeriod().count() << "ms\n";
      }
      else {
        ctx.out<< ia("expires") << "never\n";
      }
    },
    ctx.makeCommandFailureHandler("adding route"),
    ctx.makeCommandOptions());

  ctx.face.processEvents();
}

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
    if (route.hasExpirationPeriod()) {
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
    if (route.hasExpirationPeriod()) {
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
