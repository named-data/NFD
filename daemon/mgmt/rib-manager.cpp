/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#include "rib-manager.hpp"

#include "common/global.hpp"
#include "common/logger.hpp"
#include "rib/rib.hpp"
#include "table/fib.hpp"

#include <boost/asio/defer.hpp>
#include <boost/lexical_cast.hpp>

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/rib-entry.hpp>
#include <ndn-cxx/mgmt/nfd/status-dataset.hpp>
#include <ndn-cxx/security/certificate-fetcher-direct-fetch.hpp>

namespace nfd {

using rib::Route;

NFD_LOG_INIT(RibManager);

const std::string MGMT_MODULE_NAME = "rib";
const Name LOCALHOST_TOP_PREFIX = "/localhost/nfd";
constexpr time::seconds ACTIVE_FACE_FETCH_INTERVAL = 5_min;

RibManager::RibManager(rib::Rib& rib, ndn::Face& face, ndn::KeyChain& keyChain,
                       ndn::nfd::Controller& nfdController, Dispatcher& dispatcher)
  : ManagerBase(MGMT_MODULE_NAME, dispatcher)
  , m_rib(rib)
  , m_keyChain(keyChain)
  , m_nfdController(nfdController)
  , m_dispatcher(dispatcher)
  , m_faceMonitor(face)
  , m_localhostValidator(face)
  , m_localhopValidator(make_unique<ndn::security::CertificateFetcherDirectFetch>(face))
  , m_paValidator(make_unique<ndn::security::CertificateFetcherDirectFetch>(face))
  , m_isLocalhopEnabled(false)
{
  registerCommandHandler<ndn::nfd::RibRegisterCommand>([this] (auto&&, auto&&... args) {
    registerEntry(std::forward<decltype(args)>(args)...);
  });
  registerCommandHandler<ndn::nfd::RibUnregisterCommand>([this] (auto&&, auto&&... args) {
    unregisterEntry(std::forward<decltype(args)>(args)...);
  });
  registerCommandHandler<ndn::nfd::RibAnnounceCommand>([this] (auto&&, auto&&... args) {
    announceEntry(std::forward<decltype(args)>(args)...);
  });
  registerStatusDatasetHandler("list", [this] (auto&&, auto&&, auto&&... args) {
    listEntries(std::forward<decltype(args)>(args)...);
  });
}

void
RibManager::applyLocalhostConfig(const ConfigSection& section, const std::string& filename)
{
  m_localhostValidator.load(section, filename);
}

void
RibManager::enableLocalhop(const ConfigSection& section, const std::string& filename)
{
  m_localhopValidator.load(section, filename);
  m_isLocalhopEnabled = true;
}

void
RibManager::disableLocalhop()
{
  m_isLocalhopEnabled = false;
}

void
RibManager::applyPaConfig(const ConfigSection& section, const std::string& filename)
{
  m_paValidator.load(section, filename);
}

void
RibManager::registerWithNfd()
{
  registerTopPrefix(LOCALHOST_TOP_PREFIX);

  if (m_isLocalhopEnabled) {
    registerTopPrefix(LOCALHOP_TOP_PREFIX);
  }

  NFD_LOG_INFO("Start monitoring face create/destroy events");
  m_faceMonitor.onNotification.connect([this] (const auto& notif) { onNotification(notif); });
  m_faceMonitor.start();

  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

void
RibManager::enableLocalFields()
{
  m_nfdController.start<ndn::nfd::FaceUpdateCommand>(
    ControlParameters().setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true),
    [] (const ControlParameters&) {
      NFD_LOG_TRACE("Local fields enabled");
    },
    [] (const ControlResponse& res) {
      NDN_THROW(Error("Could not enable local fields (error " +
                      std::to_string(res.getCode()) + ": " + res.getText() + ")"));
    });
}

void
RibManager::addRoute(const Name& name, Route route, const time::steady_clock::time_point& now,
                     const std::function<void(RibUpdateResult)>& done)
{
  if (route.expires && *route.expires <= now) {
    m_rib.onRouteExpiration(name, route);
    if (done) {
      done(RibUpdateResult::EXPIRED);
    }
    return;
  }

  NFD_LOG_INFO("Adding route " << name << " nexthop=" << route.faceId <<
               " origin=" << route.origin << " cost=" << route.cost);

  if (route.expires) {
    auto delay = *route.expires - now;
    auto event = getScheduler().schedule(delay, [=] { m_rib.onRouteExpiration(name, route); });
    route.setExpirationEvent(std::move(event));
    // cast to milliseconds to make the logs easier to read
    NFD_LOG_TRACE("Route will expire in " << time::duration_cast<time::milliseconds>(delay));
  }

  beginRibUpdate({rib::RibUpdate::REGISTER, name, std::move(route)}, done);
}

void
RibManager::beginRibUpdate(const rib::RibUpdate& update,
                           const std::function<void(RibUpdateResult)>& done)
{
  m_rib.beginApplyUpdate(update,
    [=] {
      NFD_LOG_DEBUG(update << " -> OK");
      if (done) {
        done(RibUpdateResult::OK);
      }
    },
    [=] (uint32_t code, const std::string& error) {
      NFD_LOG_DEBUG(update << " -> error " << code << ": " << error);

      // Since the FIB rejected the update, clean up invalid routes
      scheduleActiveFaceFetch(1_s);

      if (done) {
        done(RibUpdateResult::ERROR);
      }
    });
}

void
RibManager::registerTopPrefix(const Name& topPrefix)
{
  // add FIB nexthop
  m_nfdController.start<ndn::nfd::FibAddNextHopCommand>(
    ControlParameters().setName(Name(topPrefix).append(MGMT_MODULE_NAME)).setFaceId(0),
    [=] (const ControlParameters& res) {
      NFD_LOG_DEBUG("Successfully registered " << topPrefix << " with the forwarder");

      // Routes must be inserted into the RIB so route flags can be applied
      Route route;
      route.faceId = res.getFaceId();
      route.origin = ndn::nfd::ROUTE_ORIGIN_APP;
      route.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;

      m_rib.insert(topPrefix, route);
    },
    [=] (const ControlResponse& res) {
      NDN_THROW(Error("Could not add FIB entry " + topPrefix.toUri() + " (error " +
                      std::to_string(res.getCode()) + ": " + res.getText() + ")"));
    });

  // add top prefix to the dispatcher without prefix registration
  m_dispatcher.addTopPrefix(topPrefix, false);
}

static uint64_t
getIncomingFaceId(const Interest& request)
{
  auto incomingFaceIdTag = request.getTag<lp::IncomingFaceIdTag>();
  // NDNLPv2 says "application MUST be prepared to receive a packet without IncomingFaceId field",
  // but it's fine to assert IncomingFaceId is available, because InternalFace lives inside NFD
  // and is initialized synchronously with IncomingFaceId field enabled.
  BOOST_ASSERT(incomingFaceIdTag != nullptr);
  return incomingFaceIdTag->get();
}

void
RibManager::registerEntry(const Interest& interest, ControlParameters parameters,
                          const CommandContinuation& done)
{
  if (parameters.getName().size() > Fib::getMaxDepth()) {
    done(ControlResponse(414, "Route prefix cannot exceed " + std::to_string(Fib::getMaxDepth()) +
                              " components"));
    return;
  }

  if (parameters.getFaceId() == 0) { // self registration
    parameters.setFaceId(getIncomingFaceId(interest));
  }

  // Respond since command is valid and authorized
  done(ControlResponse(200, "OK").setBody(parameters.wireEncode()));

  Route route;
  route.faceId = parameters.getFaceId();
  route.origin = parameters.getOrigin();
  route.cost = parameters.getCost();
  route.flags = parameters.getFlags();

  auto now = time::steady_clock::now();
  if (parameters.hasExpirationPeriod() &&
      parameters.getExpirationPeriod() != time::milliseconds::max()) {
    route.expires = now + parameters.getExpirationPeriod();
  }

  addRoute(parameters.getName(), std::move(route), now);
}

void
RibManager::unregisterEntry(const Interest& interest, ControlParameters parameters,
                            const CommandContinuation& done)
{
  if (parameters.getFaceId() == 0) { // self unregistration
    parameters.setFaceId(getIncomingFaceId(interest));
  }

  // Respond since command is valid and authorized
  done(ControlResponse(200, "OK").setBody(parameters.wireEncode()));

  Route route;
  route.faceId = parameters.getFaceId();
  route.origin = parameters.getOrigin();

  NFD_LOG_INFO("Removing route " << parameters.getName() <<
               " nexthop=" << route.faceId << " origin=" << route.origin);

  beginRibUpdate({rib::RibUpdate::UNREGISTER, parameters.getName(), route}, nullptr);
}

void
RibManager::announceEntry(const Interest& interest, const ndn::nfd::RibAnnounceParameters& parameters,
                          const CommandContinuation& done)
{
  const auto& announcement = parameters.getPrefixAnnouncement();
  const auto& name = announcement.getAnnouncedName();
  if (name.size() > Fib::getMaxDepth()) {
    done(ControlResponse(414, "Route prefix cannot exceed " + std::to_string(Fib::getMaxDepth()) +
                         " components"));
    return;
  }

  auto inFaceId = getIncomingFaceId(interest);

  NDN_LOG_TRACE("Validating announcement " << announcement);
  BOOST_ASSERT(announcement.getData());
  m_paValidator.validate(*announcement.getData(),
    [=] (const Data&) {
      auto now = time::steady_clock::now();
      Route route(announcement, inFaceId);

      // Prepare parameters for response
      ControlParameters responseParams;
      responseParams
        .setName(announcement.getAnnouncedName())
        .setFaceId(route.faceId)
        .setOrigin(route.origin)
        .setCost(route.cost)
        .setFlags(route.flags)
        .setExpirationPeriod(time::duration_cast<time::milliseconds>(route.annExpires - now));

      // Respond since command is valid and authorized
      done(ControlResponse(200, "OK").setBody(responseParams.wireEncode()));
      addRoute(announcement.getAnnouncedName(), std::move(route), now);
    },
    [=] (const Data&, const ndn::security::ValidationError& err) {
      NDN_LOG_DEBUG("announce " << name << " -> " << err);
      done(ControlResponse(403, "Prefix announcement rejected: " +
                           boost::lexical_cast<std::string>(err.getCode())));
    });
}

void
RibManager::listEntries(ndn::mgmt::StatusDatasetContext& context) const
{
  auto now = time::steady_clock::now();
  for (const auto& kv : m_rib) {
    const rib::RibEntry& entry = *kv.second;
    ndn::nfd::RibEntry item;
    item.setName(entry.getName());
    for (const Route& route : entry.getRoutes()) {
      ndn::nfd::Route r;
      r.setFaceId(route.faceId);
      r.setOrigin(route.origin);
      r.setCost(route.cost);
      r.setFlags(route.flags);
      if (route.expires) {
        r.setExpirationPeriod(time::duration_cast<time::milliseconds>(*route.expires - now));
      }
      item.addRoute(r);
    }
    context.append(item.wireEncode());
  }
  context.end();
}

ndn::mgmt::Authorization
RibManager::makeAuthorization(const std::string&)
{
  return [this] (const Name& prefix, const Interest& interest,
                 const ndn::mgmt::ControlParametersBase* params,
                 const ndn::mgmt::AcceptContinuation& accept,
                 const ndn::mgmt::RejectContinuation& reject) {
    BOOST_ASSERT(params != nullptr);
    BOOST_ASSERT(typeid(*params) == typeid(ndn::nfd::ControlParameters) ||
                 typeid(*params) == typeid(ndn::nfd::RibAnnounceParameters));
    BOOST_ASSERT(prefix == LOCALHOST_TOP_PREFIX || prefix == LOCALHOP_TOP_PREFIX);

    auto& validator = prefix == LOCALHOST_TOP_PREFIX ? m_localhostValidator : m_localhopValidator;
    validator.validate(interest,
                       [&interest, accept] (auto&&...) { accept(extractSigner(interest)); },
                       [reject] (auto&&...) { reject(ndn::mgmt::RejectReply::STATUS403); });
  };
}

std::ostream&
operator<<(std::ostream& os, RibManager::SlAnnounceResult res)
{
  switch (res) {
  case RibManager::SlAnnounceResult::OK:
    return os << "OK";
  case RibManager::SlAnnounceResult::ERROR:
    return os << "ERROR";
  case RibManager::SlAnnounceResult::VALIDATION_FAILURE:
    return os << "VALIDATION_FAILURE";
  case RibManager::SlAnnounceResult::EXPIRED:
    return os << "EXPIRED";
  case RibManager::SlAnnounceResult::NOT_FOUND:
    return os << "NOT_FOUND";
  }
  NDN_THROW(std::invalid_argument("Unknown SlAnnounceResult"));
}

RibManager::SlAnnounceResult
RibManager::getSlAnnounceResultFromRibUpdateResult(RibUpdateResult r)
{
  switch (r) {
  case RibUpdateResult::OK:
    return SlAnnounceResult::OK;
  case RibUpdateResult::ERROR:
    return SlAnnounceResult::ERROR;
  case RibUpdateResult::EXPIRED:
    return SlAnnounceResult::EXPIRED;
  }
  NDN_CXX_UNREACHABLE;
}

void
RibManager::slAnnounce(const ndn::PrefixAnnouncement& pa, uint64_t faceId,
                       time::milliseconds maxLifetime, const SlAnnounceCallback& cb)
{
  BOOST_ASSERT(pa.getData());

  m_paValidator.validate(*pa.getData(),
    [=] (const Data&) {
      auto now = time::steady_clock::now();
      Route route(pa, faceId);
      route.expires = std::min(route.annExpires, now + maxLifetime);
      addRoute(pa.getAnnouncedName(), std::move(route), now, [=] (RibUpdateResult ribRes) {
        auto res = getSlAnnounceResultFromRibUpdateResult(ribRes);
        NFD_LOG_INFO("slAnnounce " << pa.getAnnouncedName() << " " << faceId << " -> " << res);
        cb(res);
      });
    },
    [=] (const Data&, const ndn::security::ValidationError& err) {
      NFD_LOG_INFO("slAnnounce " << pa.getAnnouncedName() << " " << faceId <<
                   " -> validation error: " << err);
      cb(SlAnnounceResult::VALIDATION_FAILURE);
    });
}

void
RibManager::slRenew(const Name& name, uint64_t faceId, time::milliseconds maxLifetime,
                    const SlAnnounceCallback& cb)
{
  Route routeQuery;
  routeQuery.faceId = faceId;
  routeQuery.origin = ndn::nfd::ROUTE_ORIGIN_PREFIXANN;
  Route* oldRoute = m_rib.findLongestPrefix(name, routeQuery);

  if (oldRoute == nullptr || !oldRoute->announcement) {
    NFD_LOG_DEBUG("slRenew " << name << " " << faceId << " -> not found");
    return cb(SlAnnounceResult::NOT_FOUND);
  }

  auto now = time::steady_clock::now();
  Name routeName = oldRoute->announcement->getAnnouncedName();
  Route route = *oldRoute;
  route.expires = std::min(route.annExpires, now + maxLifetime);

  addRoute(routeName, std::move(route), now, [=] (RibUpdateResult ribRes) {
    auto res = getSlAnnounceResultFromRibUpdateResult(ribRes);
    NFD_LOG_INFO("slRenew " << name << " " << faceId << " -> " << res << " " << routeName);
    cb(res);
  });
}

void
RibManager::slFindAnn(const Name& name, const SlFindAnnCallback& cb) const
{
  shared_ptr<rib::RibEntry> entry;
  auto exactMatch = m_rib.find(name);
  if (exactMatch != m_rib.end()) {
    entry = exactMatch->second;
  }
  else {
    entry = m_rib.findParent(name);
  }
  if (entry == nullptr) {
    return cb(std::nullopt);
  }

  auto pa = entry->getPrefixAnnouncement();
  pa.toData(m_keyChain);
  cb(pa);
}

void
RibManager::fetchActiveFaces()
{
  NFD_LOG_DEBUG("Fetching active faces");

  m_nfdController.fetch<ndn::nfd::FaceDataset>(
    [this] (auto&&... args) { removeInvalidFaces(std::forward<decltype(args)>(args)...); },
    [this] (uint32_t code, const std::string& reason) {
      NFD_LOG_WARN("Failed to fetch face dataset (error " << code << ": " << reason << ")");
      scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
    });
}

void
RibManager::scheduleActiveFaceFetch(time::seconds timeToWait)
{
  m_activeFaceFetchEvent = getScheduler().schedule(timeToWait, [this] { fetchActiveFaces(); });
}

void
RibManager::removeInvalidFaces(const std::vector<ndn::nfd::FaceStatus>& activeFaces)
{
  NFD_LOG_DEBUG("Checking for invalid face registrations");

  std::set<uint64_t> activeIds;
  for (const auto& faceStatus : activeFaces) {
    activeIds.insert(faceStatus.getFaceId());
  }
  boost::asio::defer(getGlobalIoService(),
                     [this, active = std::move(activeIds)] { m_rib.beginRemoveFailedFaces(active); });

  // Reschedule the check for future clean up
  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

void
RibManager::onNotification(const ndn::nfd::FaceEventNotification& notification)
{
  NFD_LOG_TRACE("Received notification " << notification);

  if (notification.getKind() == ndn::nfd::FACE_EVENT_DESTROYED) {
    boost::asio::defer(getGlobalIoService(),
                       [this, id = notification.getFaceId()] { m_rib.beginRemoveFace(id); });
  }
}

} // namespace nfd
