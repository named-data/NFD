/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/mgmt/nfd/rib-entry.hpp>
#include <ndn-cxx/security/v2/certificate-fetcher-direct-fetch.hpp>

namespace nfd {

using rib::RibUpdate;
using rib::Route;

NFD_LOG_INIT(RibManager);

static const std::string MGMT_MODULE_NAME = "rib";
static const Name LOCALHOST_TOP_PREFIX = "/localhost/nfd";
static const time::seconds ACTIVE_FACE_FETCH_INTERVAL = 5_min;

const Name RibManager::LOCALHOP_TOP_PREFIX = "/localhop/nfd";

RibManager::RibManager(rib::Rib& rib, ndn::Face& face, ndn::KeyChain& keyChain,
                       ndn::nfd::Controller& nfdController, Dispatcher& dispatcher)
  : ManagerBase(MGMT_MODULE_NAME, dispatcher)
  , m_rib(rib)
  , m_keyChain(keyChain)
  , m_nfdController(nfdController)
  , m_dispatcher(dispatcher)
  , m_faceMonitor(face)
  , m_localhostValidator(face)
  , m_localhopValidator(make_unique<ndn::security::v2::CertificateFetcherDirectFetch>(face))
  , m_paValidator(make_unique<ndn::security::v2::CertificateFetcherDirectFetch>(face))
  , m_isLocalhopEnabled(false)
{
  registerCommandHandler<ndn::nfd::RibRegisterCommand>("register",
    bind(&RibManager::registerEntry, this, _2, _3, _4, _5));
  registerCommandHandler<ndn::nfd::RibUnregisterCommand>("unregister",
    bind(&RibManager::unregisterEntry, this, _2, _3, _4, _5));

  registerStatusDatasetHandler("list", bind(&RibManager::listEntries, this, _1, _2, _3));
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
  m_faceMonitor.onNotification.connect(bind(&RibManager::onNotification, this, _1));
  m_faceMonitor.start();

  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

void
RibManager::enableLocalFields()
{
  m_nfdController.start<ndn::nfd::FaceUpdateCommand>(
    ControlParameters().setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true),
    [] (const ControlParameters&) {
      NFD_LOG_DEBUG("Local fields enabled");
    },
    [] (const ControlResponse& res) {
      NDN_THROW(Error("Couldn't enable local fields (" + to_string(res.getCode()) +
                      " " + res.getText() + ")"));
    });
}

void
RibManager::beginAddRoute(const Name& name, Route route, optional<time::nanoseconds> expires,
                          const std::function<void(RibUpdateResult)>& done)
{
  if (expires) {
    route.expires = time::steady_clock::now() + *expires;
  }
  else if (route.expires) {
    expires = *route.expires - time::steady_clock::now();
  }

  if (expires && *expires <= 0_s) {
    m_rib.onRouteExpiration(name, route);
    return done(RibUpdateResult::EXPIRED);
  }

  NFD_LOG_INFO("Adding route " << name << " nexthop=" << route.faceId <<
               " origin=" << route.origin << " cost=" << route.cost);

  if (expires) {
    auto event = getScheduler().schedule(*expires, [=] { m_rib.onRouteExpiration(name, route); });
    route.setExpirationEvent(event);
    NFD_LOG_TRACE("Scheduled unregistration at: " << *route.expires);
  }

  RibUpdate update;
  update.setAction(RibUpdate::REGISTER)
        .setName(name)
        .setRoute(route);
  beginRibUpdate(update, done);
}

void
RibManager::beginRemoveRoute(const Name& name, const Route& route,
                             const std::function<void(RibUpdateResult)>& done)
{
  NFD_LOG_INFO("Removing route " << name << " nexthop=" << route.faceId <<
               " origin=" << route.origin);

  RibUpdate update;
  update.setAction(RibUpdate::UNREGISTER)
        .setName(name)
        .setRoute(route);
  beginRibUpdate(update, done);
}

void
RibManager::beginRibUpdate(const RibUpdate& update,
                           const std::function<void(RibUpdateResult)>& done)
{
  m_rib.beginApplyUpdate(update,
    [=] {
      NFD_LOG_DEBUG("RIB update succeeded for " << update);
      done(RibUpdateResult::OK);
    },
    [=] (uint32_t code, const std::string& error) {
      NFD_LOG_DEBUG("RIB update failed for " << update << " (" << code << " " << error << ")");

      // Since the FIB rejected the update, clean up invalid routes
      scheduleActiveFaceFetch(1_s);

      done(RibUpdateResult::ERROR);
    });
}

void
RibManager::registerTopPrefix(const Name& topPrefix)
{
  // add FIB nexthop
  m_nfdController.start<ndn::nfd::FibAddNextHopCommand>(
    ControlParameters().setName(Name(topPrefix).append(MGMT_MODULE_NAME))
                       .setFaceId(0),
    [=] (const ControlParameters& res) {
      NFD_LOG_DEBUG("Successfully registered " << topPrefix << " with NFD");

      // Routes must be inserted into the RIB so route flags can be applied
      Route route;
      route.faceId = res.getFaceId();
      route.origin = ndn::nfd::ROUTE_ORIGIN_APP;
      route.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;

      m_rib.insert(topPrefix, route);
    },
    [=] (const ControlResponse& res) {
      NDN_THROW(Error("Cannot add FIB entry " + topPrefix.toUri() + " (" +
                      to_string(res.getCode()) + " " + res.getText() + ")"));
    });

  // add top prefix to the dispatcher without prefix registration
  m_dispatcher.addTopPrefix(topPrefix, false);
}

void
RibManager::registerEntry(const Name& topPrefix, const Interest& interest,
                          ControlParameters parameters,
                          const ndn::mgmt::CommandContinuation& done)
{
  if (parameters.getName().size() > Fib::getMaxDepth()) {
    done(ControlResponse(414, "Route prefix cannot exceed " + to_string(Fib::getMaxDepth()) +
                              " components"));
    return;
  }

  setFaceForSelfRegistration(interest, parameters);

  // Respond since command is valid and authorized
  done(ControlResponse(200, "Success").setBody(parameters.wireEncode()));

  Route route;
  route.faceId = parameters.getFaceId();
  route.origin = parameters.getOrigin();
  route.cost = parameters.getCost();
  route.flags = parameters.getFlags();

  optional<time::nanoseconds> expires;
  if (parameters.hasExpirationPeriod() &&
      parameters.getExpirationPeriod() != time::milliseconds::max()) {
    expires = time::duration_cast<time::nanoseconds>(parameters.getExpirationPeriod());
  }

  beginAddRoute(parameters.getName(), std::move(route), expires, [] (RibUpdateResult) {});
}

void
RibManager::unregisterEntry(const Name& topPrefix, const Interest& interest,
                            ControlParameters parameters,
                            const ndn::mgmt::CommandContinuation& done)
{
  setFaceForSelfRegistration(interest, parameters);

  // Respond since command is valid and authorized
  done(ControlResponse(200, "Success").setBody(parameters.wireEncode()));

  Route route;
  route.faceId = parameters.getFaceId();
  route.origin = parameters.getOrigin();

  beginRemoveRoute(parameters.getName(), route, [] (RibUpdateResult) {});
}

void
RibManager::listEntries(const Name& topPrefix, const Interest& interest,
                        ndn::mgmt::StatusDatasetContext& context)
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

void
RibManager::setFaceForSelfRegistration(const Interest& request, ControlParameters& parameters)
{
  bool isSelfRegistration = (parameters.getFaceId() == 0);
  if (isSelfRegistration) {
    shared_ptr<lp::IncomingFaceIdTag> incomingFaceIdTag = request.getTag<lp::IncomingFaceIdTag>();
    // NDNLPv2 says "application MUST be prepared to receive a packet without IncomingFaceId field",
    // but it's fine to assert IncomingFaceId is available, because InternalFace lives inside NFD
    // and is initialized synchronously with IncomingFaceId field enabled.
    BOOST_ASSERT(incomingFaceIdTag != nullptr);
    parameters.setFaceId(*incomingFaceIdTag);
  }
}

ndn::mgmt::Authorization
RibManager::makeAuthorization(const std::string& verb)
{
  return [this] (const Name& prefix, const Interest& interest,
                 const ndn::mgmt::ControlParameters* params,
                 const ndn::mgmt::AcceptContinuation& accept,
                 const ndn::mgmt::RejectContinuation& reject) {
    BOOST_ASSERT(params != nullptr);
    BOOST_ASSERT(typeid(*params) == typeid(ndn::nfd::ControlParameters));
    BOOST_ASSERT(prefix == LOCALHOST_TOP_PREFIX || prefix == LOCALHOP_TOP_PREFIX);

    auto& validator = prefix == LOCALHOST_TOP_PREFIX ? m_localhostValidator : m_localhopValidator;
    validator.validate(interest,
                       bind([&interest, this, accept] { extractRequester(interest, accept); }),
                       bind([reject] { reject(ndn::mgmt::RejectReply::STATUS403); }));
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
      Route route(pa, faceId);
      route.expires = std::min(route.annExpires, time::steady_clock::now() + maxLifetime);
      beginAddRoute(pa.getAnnouncedName(), route, nullopt,
        [=] (RibUpdateResult ribRes) {
          auto res = getSlAnnounceResultFromRibUpdateResult(ribRes);
          NFD_LOG_INFO("slAnnounce " << pa.getAnnouncedName() << " " << faceId << ": " << res);
          cb(res);
        });
    },
    [=] (const Data&, ndn::security::v2::ValidationError err) {
      NFD_LOG_INFO("slAnnounce " << pa.getAnnouncedName() << " " << faceId <<
                   " validation error: " << err);
      cb(SlAnnounceResult::VALIDATION_FAILURE);
    }
  );
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
    NFD_LOG_DEBUG("slRenew " << name << " " << faceId << ": not found");
    return cb(SlAnnounceResult::NOT_FOUND);
  }
  Name routeName = oldRoute->announcement->getAnnouncedName();

  Route route = *oldRoute;
  route.expires = std::min(route.annExpires, time::steady_clock::now() + maxLifetime);
  beginAddRoute(routeName, route, nullopt,
    [=] (RibUpdateResult ribRes) {
      auto res = getSlAnnounceResultFromRibUpdateResult(ribRes);
      NFD_LOG_INFO("slRenew " << name << " " << faceId << ": " << res << " " << routeName);
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
    return cb(nullopt);
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
    bind(&RibManager::removeInvalidFaces, this, _1),
    bind(&RibManager::onFetchActiveFacesFailure, this, _1, _2),
    ndn::nfd::CommandOptions());
}

void
RibManager::onFetchActiveFacesFailure(uint32_t code, const std::string& reason)
{
  NFD_LOG_DEBUG("Face Status Dataset request failure " << code << " " << reason);
  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

void
RibManager::scheduleActiveFaceFetch(const time::seconds& timeToWait)
{
  m_activeFaceFetchEvent = getScheduler().schedule(timeToWait, [this] { fetchActiveFaces(); });
}

void
RibManager::removeInvalidFaces(const std::vector<ndn::nfd::FaceStatus>& activeFaces)
{
  NFD_LOG_DEBUG("Checking for invalid face registrations");

  std::set<uint64_t> activeFaceIds;
  for (const auto& faceStatus : activeFaces) {
    activeFaceIds.insert(faceStatus.getFaceId());
  }
  getGlobalIoService().post([=] { m_rib.beginRemoveFailedFaces(activeFaceIds); });

  // Reschedule the check for future clean up
  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

void
RibManager::onNotification(const ndn::nfd::FaceEventNotification& notification)
{
  NFD_LOG_TRACE("onNotification: " << notification);

  if (notification.getKind() == ndn::nfd::FACE_EVENT_DESTROYED) {
    NFD_LOG_DEBUG("Received notification for destroyed FaceId " << notification.getFaceId());
    getGlobalIoService().post([this, id = notification.getFaceId()] { m_rib.beginRemoveFace(id); });
  }
}

} // namespace nfd
