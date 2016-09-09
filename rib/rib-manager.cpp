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

#include "rib-manager.hpp"
#include "core/global-io.hpp"
#include "core/logger.hpp"
#include "core/scheduler.hpp"
#include <ndn-cxx/lp/tags.hpp>
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

RibManager::~RibManager()
{
  scheduler::cancel(m_activeFaceFetchEvent);
}

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
RibManager::enableLocalControlHeader()
{
  m_nfdController.start<ndn::nfd::FaceEnableLocalControlCommand>(
    ControlParameters()
      .setLocalControlFeature(ndn::nfd::LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID),
    bind(&RibManager::onControlHeaderSuccess, this),
    bind(&RibManager::onControlHeaderError, this, _1));
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
    else {
      BOOST_THROW_EXCEPTION(Error("Unrecognized rib property: " + item.first));
    }
  }

  if (!isAutoPrefixPropagatorEnabled) {
    m_prefixPropagator.disable();
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
  setFaceForSelfRegistration(interest, parameters);

  // Respond since command is valid and authorized
  done(ControlResponse(200, "Success").setBody(parameters.wireEncode()));

  Route route;
  route.faceId = parameters.getFaceId();
  route.origin = parameters.getOrigin();
  route.cost = parameters.getCost();
  route.flags = parameters.getFlags();

  if (parameters.hasExpirationPeriod() &&
      parameters.getExpirationPeriod() != time::milliseconds::max())
  {
    route.expires = time::steady_clock::now() + parameters.getExpirationPeriod();

    // Schedule a new event, the old one will be cancelled during rib insertion.
    scheduler::EventId eventId = scheduler::schedule(parameters.getExpirationPeriod(),
      bind(&Rib::onRouteExpiration, &m_rib, parameters.getName(), route));

    NFD_LOG_TRACE("Scheduled unregistration at: " << route.expires <<
                  " with EventId: " << eventId);

    // Set the  NewEventId of this entry
    route.setExpirationEvent(eventId);
  }
  else {
    route.expires = time::steady_clock::TimePoint::max();
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
  for (auto&& ribTableEntry : m_rib) {
    const auto& ribEntry = *ribTableEntry.second;
    ndn::nfd::RibEntry record;

    for (auto&& route : ribEntry) {
      ndn::nfd::Route routeElement;
      routeElement.setFaceId(route.faceId)
              .setOrigin(route.origin)
              .setCost(route.cost)
              .setFlags(route.flags);

      if (route.expires < time::steady_clock::TimePoint::max()) {
        routeElement.setExpirationPeriod(time::duration_cast<time::milliseconds>(
          route.expires - time::steady_clock::now()));
      }

      record.addRoute(routeElement);
    }

    record.setName(ribEntry.getName());
    context.append(record.wireEncode());
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

  Interest interest(FACES_LIST_DATASET_PREFIX);
  interest.setChildSelector(1);
  interest.setMustBeFresh(true);

  shared_ptr<ndn::OBufferStream> buffer = make_shared<ndn::OBufferStream>();

  m_face.expressInterest(interest,
                         bind(&RibManager::fetchSegments, this, _2, buffer),
                         bind(&RibManager::onFetchFaceStatusTimeout, this));
}

void
RibManager::fetchSegments(const Data& data, shared_ptr<ndn::OBufferStream> buffer)
{
  buffer->write(reinterpret_cast<const char*>(data.getContent().value()),
                data.getContent().value_size());

  uint64_t currentSegment = data.getName().get(-1).toSegment();

  const name::Component& finalBlockId = data.getMetaInfo().getFinalBlockId();

  if (finalBlockId.empty() || finalBlockId.toSegment() > currentSegment) {
    m_face.expressInterest(data.getName().getPrefix(-1).appendSegment(currentSegment+1),
                           bind(&RibManager::fetchSegments, this, _2, buffer),
                           bind(&RibManager::onFetchFaceStatusTimeout, this));
  }
  else {
    removeInvalidFaces(buffer);
  }
}

void
RibManager::onFetchFaceStatusTimeout()
{
  std::cerr << "Face Status Dataset request timed out" << std::endl;
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
  scheduler::cancel(m_activeFaceFetchEvent);

  m_activeFaceFetchEvent = scheduler::schedule(timeToWait,
                                               bind(&RibManager::fetchActiveFaces, this));
}

void
RibManager::removeInvalidFaces(shared_ptr<ndn::OBufferStream> buffer)
{
  NFD_LOG_DEBUG("Checking for invalid face registrations");

  ndn::ConstBufferPtr buf = buffer->buf();

  Block block;
  size_t offset = 0;
  FaceIdSet activeFaces;

  while (offset < buf->size()) {
    bool isOk = false;
    std::tie(isOk, block) = Block::fromBuffer(buf, offset);
    if (!isOk) {
      std::cerr << "ERROR: cannot decode FaceStatus TLV" << std::endl;
      break;
    }

    offset += block.size();

    ndn::nfd::FaceStatus status(block);
    activeFaces.insert(status.getFaceId());
  }

  // Look for face IDs that were registered but not active to find missed
  // face destroyed events
  for (auto&& faceId : m_registeredFaces) {
    if (activeFaces.find(faceId) == activeFaces.end()) {
      NFD_LOG_DEBUG("Removing invalid face ID: " << faceId);

      scheduler::schedule(time::seconds(0),
                          bind(&RibManager::onFaceDestroyedEvent, this, faceId));
    }
  }

  // Reschedule the check for future clean up
  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

void
RibManager::onNotification(const FaceEventNotification& notification)
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
  route.expires = time::steady_clock::TimePoint::max();
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
RibManager::onControlHeaderSuccess()
{
  NFD_LOG_DEBUG("Local control header enabled");
}

void
RibManager::onControlHeaderError(const ndn::nfd::ControlResponse& response)
{
  std::ostringstream os;
  os << "Couldn't enable local control header "
     << "(code: " << response.getCode() << ", info: " << response.getText() << ")";
  BOOST_THROW_EXCEPTION(Error(os.str()));
}

} // namespace rib
} // namespace nfd
