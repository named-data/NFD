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

#include "rib-manager.hpp"
#include "readvertise/readvertise.hpp"
#include "readvertise/client-to-nlsr-readvertise-policy.hpp"
#include "readvertise/nfd-rib-readvertise-destination.hpp"

#include "core/fib-max-depth.hpp"
#include "core/logger.hpp"
#include "core/scheduler.hpp"

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>
#include <ndn-cxx/mgmt/nfd/control-response.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/mgmt/nfd/rib-entry.hpp>

namespace nfd {
namespace rib {

NFD_LOG_INIT("RibManager");

const Name RibManager::LOCAL_HOST_TOP_PREFIX = "/localhost/nfd";
const Name RibManager::LOCAL_HOP_TOP_PREFIX = "/localhop/nfd";
const std::string RibManager::MGMT_MODULE_NAME = "rib";
const Name RibManager::FACES_LIST_DATASET_PREFIX = "/localhost/nfd/faces/list";
const time::seconds RibManager::ACTIVE_FACE_FETCH_INTERVAL = time::seconds(300);
const Name RibManager::READVERTISE_NLSR_PREFIX = "/localhost/nlsr";

RibManager::RibManager(Dispatcher& dispatcher,
                       ndn::Face& face,
                       ndn::KeyChain& keyChain)
  : ManagerBase(dispatcher, MGMT_MODULE_NAME)
  , m_face(face)
  , m_keyChain(keyChain)
  , m_nfdController(m_face, m_keyChain)
  , m_faceMonitor(m_face)
  , m_localhostValidator(m_face)
  , m_localhopValidator(m_face)
  , m_isLocalhopEnabled(false)
  , m_prefixPropagator(m_nfdController, m_keyChain, m_rib)
  , m_fibUpdater(m_rib, m_nfdController)
  , m_addTopPrefix([&dispatcher] (const Name& topPrefix) {
      dispatcher.addTopPrefix(topPrefix, false);
    })
{
  registerCommandHandler<ndn::nfd::RibRegisterCommand>("register",
    bind(&RibManager::registerEntry, this, _2, _3, _4, _5));
  registerCommandHandler<ndn::nfd::RibUnregisterCommand>("unregister",
    bind(&RibManager::unregisterEntry, this, _2, _3, _4, _5));

  registerStatusDatasetHandler("list", bind(&RibManager::listEntries, this, _1, _2, _3));
}

RibManager::~RibManager() = default;

void
RibManager::registerWithNfd()
{
  registerTopPrefix(LOCAL_HOST_TOP_PREFIX);

  if (m_isLocalhopEnabled) {
    registerTopPrefix(LOCAL_HOP_TOP_PREFIX);
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
    ControlParameters()
      .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true),
    bind(&RibManager::onEnableLocalFieldsSuccess, this),
    bind(&RibManager::onEnableLocalFieldsError, this, _1));
}

void
RibManager::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("rib",
                               bind(&RibManager::onConfig, this, _1, _2, _3));
}

void
RibManager::onRibUpdateSuccess(const RibUpdate& update)
{
  NFD_LOG_DEBUG("RIB update succeeded for " << update);
}

void
RibManager::onRibUpdateFailure(const RibUpdate& update, uint32_t code, const std::string& error)
{
  NFD_LOG_DEBUG("RIB update failed for " << update << " (code: " << code
                                                   << ", error: " << error << ")");

  // Since the FIB rejected the update, clean up invalid routes
  scheduleActiveFaceFetch(time::seconds(1));
}

void
RibManager::onConfig(const ConfigSection& configSection,
                     bool isDryRun,
                     const std::string& filename)
{
  bool isAutoPrefixPropagatorEnabled = false;
  bool wantReadvertiseToNlsr = false;

  for (const auto& item : configSection) {
    if (item.first == "localhost_security") {
      m_localhostValidator.load(item.second, filename);
    }
    else if (item.first == "localhop_security") {
      m_localhopValidator.load(item.second, filename);
      m_isLocalhopEnabled = true;
    }
    else if (item.first == "auto_prefix_propagate") {
      m_prefixPropagator.loadConfig(item.second);
      isAutoPrefixPropagatorEnabled = true;

      // Avoid other actions when isDryRun == true
      if (isDryRun) {
        continue;
      }

      m_prefixPropagator.enable();
    }
    else if (item.first == "readvertise_nlsr") {
      wantReadvertiseToNlsr = ConfigFile::parseYesNo(item, "rib.readvertise_nlsr");
    }
    else {
      BOOST_THROW_EXCEPTION(Error("Unrecognized rib property: " + item.first));
    }
  }

  if (!isAutoPrefixPropagatorEnabled) {
    m_prefixPropagator.disable();
  }

  if (wantReadvertiseToNlsr && m_readvertiseNlsr == nullptr) {
    NFD_LOG_DEBUG("Enabling readvertise-to-nlsr.");
    m_readvertiseNlsr.reset(new Readvertise(
      m_rib,
      make_unique<ClientToNlsrReadvertisePolicy>(),
      make_unique<NfdRibReadvertiseDestination>(m_nfdController, READVERTISE_NLSR_PREFIX, m_rib)));
  }
  else if (!wantReadvertiseToNlsr && m_readvertiseNlsr != nullptr) {
    NFD_LOG_DEBUG("Disabling readvertise-to-nlsr.");
    m_readvertiseNlsr.reset();
  }
}

void
RibManager::registerTopPrefix(const Name& topPrefix)
{
  // register entry to the FIB
  m_nfdController.start<ndn::nfd::FibAddNextHopCommand>(
     ControlParameters()
       .setName(Name(topPrefix).append(MGMT_MODULE_NAME))
       .setFaceId(0),
     bind(&RibManager::onCommandPrefixAddNextHopSuccess, this, cref(topPrefix), _1),
     bind(&RibManager::onCommandPrefixAddNextHopError, this, cref(topPrefix), _1));

  // add top prefix to the dispatcher
  m_addTopPrefix(topPrefix);
}

void
RibManager::registerEntry(const Name& topPrefix, const Interest& interest,
                          ControlParameters parameters,
                          const ndn::mgmt::CommandContinuation& done)
{
  if (parameters.getName().size() > FIB_MAX_DEPTH) {
    done(ControlResponse(414, "Route prefix cannot exceed " + ndn::to_string(FIB_MAX_DEPTH) +
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

  if (parameters.hasExpirationPeriod() &&
      parameters.getExpirationPeriod() != time::milliseconds::max()) {
    route.expires = time::steady_clock::now() + parameters.getExpirationPeriod();

    // Schedule a new event, the old one will be cancelled during rib insertion.
    scheduler::EventId eventId = scheduler::schedule(parameters.getExpirationPeriod(),
      bind(&Rib::onRouteExpiration, &m_rib, parameters.getName(), route));

    NFD_LOG_TRACE("Scheduled unregistration at: " << *route.expires <<
                  " with EventId: " << eventId);

    // Set the  NewEventId of this entry
    route.setExpirationEvent(eventId);
  }
  else {
    route.expires = nullopt;
  }

  NFD_LOG_INFO("Adding route " << parameters.getName() << " nexthop=" << route.faceId
                                                       << " origin=" << route.origin
                                                       << " cost=" << route.cost);

  RibUpdate update;
  update.setAction(RibUpdate::REGISTER)
        .setName(parameters.getName())
        .setRoute(route);

  m_rib.beginApplyUpdate(update,
                         bind(&RibManager::onRibUpdateSuccess, this, update),
                         bind(&RibManager::onRibUpdateFailure, this, update, _1, _2));

  m_registeredFaces.insert(route.faceId);
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

  NFD_LOG_INFO("Removing route " << parameters.getName() << " nexthop=" << route.faceId
                                                         << " origin=" << route.origin);

  RibUpdate update;
  update.setAction(RibUpdate::UNREGISTER)
        .setName(parameters.getName())
        .setRoute(route);

  m_rib.beginApplyUpdate(update,
                         bind(&RibManager::onRibUpdateSuccess, this, update),
                         bind(&RibManager::onRibUpdateFailure, this, update, _1, _2));
}

void
RibManager::listEntries(const Name& topPrefix, const Interest& interest,
                        ndn::mgmt::StatusDatasetContext& context)
{
  auto now = time::steady_clock::now();
  for (const auto& kv : m_rib) {
    const RibEntry& entry = *kv.second;
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
    BOOST_ASSERT(prefix == LOCAL_HOST_TOP_PREFIX || prefix == LOCAL_HOP_TOP_PREFIX);

    ndn::ValidatorConfig& validator = prefix == LOCAL_HOST_TOP_PREFIX ?
                                      m_localhostValidator : m_localhopValidator;
    validator.validate(interest,
                       bind([&interest, this, accept] { extractRequester(interest, accept); }),
                       bind([reject] { reject(ndn::mgmt::RejectReply::STATUS403); }));
  };
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
RibManager::onFaceDestroyedEvent(uint64_t faceId)
{
  m_rib.beginRemoveFace(faceId);
  m_registeredFaces.erase(faceId);
}

void
RibManager::scheduleActiveFaceFetch(const time::seconds& timeToWait)
{
  m_activeFaceFetchEvent = scheduler::schedule(timeToWait, [this] { this->fetchActiveFaces(); });
}

void
RibManager::removeInvalidFaces(const std::vector<ndn::nfd::FaceStatus>& activeFaces)
{
  NFD_LOG_DEBUG("Checking for invalid face registrations");

  FaceIdSet activeFaceIds;
  for (const auto& faceStatus : activeFaces) {
    activeFaceIds.insert(faceStatus.getFaceId());
  }

  // Look for face IDs that were registered but not active to find missed
  // face destroyed events
  for (auto faceId : m_registeredFaces) {
    if (activeFaceIds.count(faceId) == 0) {
      NFD_LOG_DEBUG("Removing invalid face ID: " << faceId);
      scheduler::schedule(time::seconds(0), [this, faceId] { this->onFaceDestroyedEvent(faceId); });
    }
  }

  // Reschedule the check for future clean up
  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

void
RibManager::onNotification(const ndn::nfd::FaceEventNotification& notification)
{
  NFD_LOG_TRACE("onNotification: " << notification);

  if (notification.getKind() == ndn::nfd::FACE_EVENT_DESTROYED) {
    NFD_LOG_DEBUG("Received notification for destroyed faceId: " << notification.getFaceId());

    scheduler::schedule(time::seconds(0),
                        bind(&RibManager::onFaceDestroyedEvent, this, notification.getFaceId()));
  }
}

void
RibManager::onCommandPrefixAddNextHopSuccess(const Name& prefix,
                                             const ndn::nfd::ControlParameters& result)
{
  NFD_LOG_DEBUG("Successfully registered " + prefix.toUri() + " with NFD");

  // Routes must be inserted into the RIB so route flags can be applied
  Route route;
  route.faceId = result.getFaceId();
  route.origin = ndn::nfd::ROUTE_ORIGIN_APP;
  route.expires = nullopt;
  route.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;

  m_rib.insert(prefix, route);

  m_registeredFaces.insert(route.faceId);
}

void
RibManager::onCommandPrefixAddNextHopError(const Name& name,
                                           const ndn::nfd::ControlResponse& response)
{
  BOOST_THROW_EXCEPTION(Error("Error in setting interest filter (" + name.toUri() +
                              "): " + response.getText()));
}

void
RibManager::onEnableLocalFieldsSuccess()
{
  NFD_LOG_DEBUG("Local fields enabled");
}

void
RibManager::onEnableLocalFieldsError(const ndn::nfd::ControlResponse& response)
{
  BOOST_THROW_EXCEPTION(Error("Couldn't enable local fields (code: " +
                              to_string(response.getCode()) + ", info: " + response.getText() +
                              ")"));
}

} // namespace rib
} // namespace nfd
