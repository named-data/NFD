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

#include "rib-module.hpp"
#include "find-face.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

void
RibModule::registerCommands(CommandParser& parser)
{
  CommandDefinition defRouteList("route", "list");
  defRouteList
    .setTitle("print RIB routes")
    .addArg("nexthop", ArgValueType::FACE_ID_OR_URI, Required::NO, Positional::YES)
    .addArg("origin", ArgValueType::ROUTE_ORIGIN, Required::NO, Positional::NO);
  parser.addCommand(defRouteList, &RibModule::list);
  parser.addAlias("route", "list", "");

  CommandDefinition defRouteShow("route", "show");
  defRouteShow
    .setTitle("show routes toward a prefix")
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES);
  parser.addCommand(defRouteShow, &RibModule::show);

  CommandDefinition defRouteAdd("route", "add");
  defRouteAdd
    .setTitle("add a route")
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES)
    .addArg("nexthop", ArgValueType::FACE_ID_OR_URI, Required::YES, Positional::YES)
    .addArg("origin", ArgValueType::ROUTE_ORIGIN, Required::NO, Positional::NO)
    .addArg("cost", ArgValueType::UNSIGNED, Required::NO, Positional::NO)
    .addArg("no-inherit", ArgValueType::NONE, Required::NO, Positional::NO)
    .addArg("capture", ArgValueType::NONE, Required::NO, Positional::NO)
    .addArg("expires", ArgValueType::UNSIGNED, Required::NO, Positional::NO);
  parser.addCommand(defRouteAdd, &RibModule::add);

  CommandDefinition defRouteRemove("route", "remove");
  defRouteRemove
    .setTitle("remove a route")
    .addArg("prefix", ArgValueType::NAME, Required::YES, Positional::YES)
    .addArg("nexthop", ArgValueType::FACE_ID_OR_URI, Required::YES, Positional::YES)
    .addArg("origin", ArgValueType::ROUTE_ORIGIN, Required::NO, Positional::NO);
  parser.addCommand(defRouteRemove, &RibModule::remove);
}

void
RibModule::list(ExecuteContext& ctx)
{
  auto nexthopIt = ctx.args.find("nexthop");
  std::set<uint64_t> nexthops;
  auto origin = ctx.args.getOptional<RouteOrigin>("origin");

  if (nexthopIt != ctx.args.end()) {
    FindFace findFace(ctx);
    FindFace::Code res = findFace.execute(nexthopIt->second, true);

    ctx.exitCode = static_cast<int>(res);
    switch (res) {
      case FindFace::Code::OK:
        break;
      case FindFace::Code::ERROR:
      case FindFace::Code::CANONIZE_ERROR:
      case FindFace::Code::NOT_FOUND:
        ctx.err << findFace.getErrorReason() << '\n';
        return;
      default:
        BOOST_ASSERT_MSG(false, "unexpected FindFace result");
        return;
    }

    nexthops = findFace.getFaceIds();
  }

  listRoutesImpl(ctx, [&] (const RibEntry& entry, const Route& route) {
    return (nexthops.empty() || nexthops.count(route.getFaceId()) > 0) &&
           (!origin || route.getOrigin() == *origin);
  });
}

void
RibModule::show(ExecuteContext& ctx)
{
  auto prefix = ctx.args.get<Name>("prefix");

  listRoutesImpl(ctx, [&] (const RibEntry& entry, const Route& route) {
    return entry.getName() == prefix;
  });
}

void
RibModule::listRoutesImpl(ExecuteContext& ctx, const RoutePredicate& filter)
{
  ctx.controller.fetch<ndn::nfd::RibDataset>(
    [&] (const std::vector<RibEntry>& dataset) {
      bool hasRoute = false;
      for (const RibEntry& entry : dataset) {
        for (const Route& route : entry.getRoutes()) {
          if (filter(entry, route)) {
            hasRoute = true;
            formatRouteText(ctx.out, entry, route, true);
            ctx.out << '\n';
          }
        }
      }

      if (!hasRoute) {
        ctx.exitCode = 6;
        ctx.err << "Route not found\n";
      }
    },
    ctx.makeDatasetFailureHandler("RIB dataset"),
    ctx.makeCommandOptions());

  ctx.face.processEvents();
}

void
RibModule::add(ExecuteContext& ctx)
{
  auto prefix = ctx.args.get<Name>("prefix");
  const boost::any& nexthop = ctx.args.at("nexthop");
  auto origin = ctx.args.get<RouteOrigin>("origin", ndn::nfd::ROUTE_ORIGIN_STATIC);
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
    .setFlags((wantChildInherit ? ndn::nfd::ROUTE_FLAG_CHILD_INHERIT : ndn::nfd::ROUTE_FLAGS_NONE) |
              (wantCapture ? ndn::nfd::ROUTE_FLAG_CAPTURE : ndn::nfd::ROUTE_FLAGS_NONE));
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
        ctx.out << ia("expires") << text::formatDuration<time::milliseconds>(resp.getExpirationPeriod()) << "\n";
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
RibModule::remove(ExecuteContext& ctx)
{
  auto prefix = ctx.args.get<Name>("prefix");
  const boost::any& nexthop = ctx.args.at("nexthop");
  auto origin = ctx.args.get<RouteOrigin>("origin", ndn::nfd::ROUTE_ORIGIN_STATIC);

  FindFace findFace(ctx);
  FindFace::Code res = findFace.execute(nexthop, true);

  ctx.exitCode = static_cast<int>(res);
  switch (res) {
    case FindFace::Code::OK:
      break;
    case FindFace::Code::ERROR:
    case FindFace::Code::CANONIZE_ERROR:
    case FindFace::Code::NOT_FOUND:
      ctx.err << findFace.getErrorReason() << '\n';
      return;
    default:
      BOOST_ASSERT_MSG(false, "unexpected FindFace result");
      return;
  }

  for (uint64_t faceId : findFace.getFaceIds()) {
    ControlParameters unregisterParams;
    unregisterParams
      .setName(prefix)
      .setFaceId(faceId)
      .setOrigin(origin);

    ctx.controller.start<ndn::nfd::RibUnregisterCommand>(
      unregisterParams,
      [&] (const ControlParameters& resp) {
        ctx.out << "route-removed ";
        text::ItemAttributes ia;
        ctx.out << ia("prefix") << resp.getName()
                << ia("nexthop") << resp.getFaceId()
                << ia("origin") << resp.getOrigin()
                << '\n';
      },
      ctx.makeCommandFailureHandler("removing route"),
      ctx.makeCommandOptions());
  }

  ctx.face.processEvents();
}

void
RibModule::fetchStatus(Controller& controller,
                       const std::function<void()>& onSuccess,
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
         << xml::formatDuration(time::duration_cast<time::seconds>(route.getExpirationPeriod()))
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
    os << "  ";
    formatEntryText(os, item);
    os << '\n';
  }
}

void
RibModule::formatEntryText(std::ostream& os, const RibEntry& entry)
{
  os << entry.getName() << " routes={";

  text::Separator sep(", ");
  for (const Route& route : entry.getRoutes()) {
    os << sep;
    formatRouteText(os, entry, route, false);
  }

  os << "}";
}

void
RibModule::formatRouteText(std::ostream& os, const RibEntry& entry, const Route& route,
                           bool includePrefix)
{
  text::ItemAttributes ia;

  if (includePrefix) {
    os << ia("prefix") << entry.getName();
  }
  os << ia("nexthop") << route.getFaceId();
  os << ia("origin") << route.getOrigin();
  os << ia("cost") << route.getCost();
  os << ia("flags") << static_cast<ndn::nfd::RouteFlags>(route.getFlags());
  if (route.hasExpirationPeriod()) {
    os << ia("expires") << text::formatDuration<time::seconds>(route.getExpirationPeriod());
  }
  else {
    os << ia("expires") << "never";
  }
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
