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

#include "face-manager.hpp"
#include "face/generic-link-service.hpp"
#include "face/protocol-factory.hpp"
#include "fw/face-table.hpp"

#include <boost/logic/tribool.hpp>

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/channel-status.hpp>

namespace nfd {

NFD_LOG_INIT("FaceManager");

FaceManager::FaceManager(FaceSystem& faceSystem,
                         Dispatcher& dispatcher,
                         CommandAuthenticator& authenticator)
  : NfdManagerBase(dispatcher, authenticator, "faces")
  , m_faceSystem(faceSystem)
  , m_faceTable(faceSystem.getFaceTable())
{
  // register handlers for ControlCommand
  registerCommandHandler<ndn::nfd::FaceCreateCommand>("create",
    bind(&FaceManager::createFace, this, _2, _3, _4, _5));

  registerCommandHandler<ndn::nfd::FaceUpdateCommand>("update",
    bind(&FaceManager::updateFace, this, _2, _3, _4, _5));

  registerCommandHandler<ndn::nfd::FaceDestroyCommand>("destroy",
    bind(&FaceManager::destroyFace, this, _2, _3, _4, _5));

  // register handlers for StatusDataset
  registerStatusDatasetHandler("list", bind(&FaceManager::listFaces, this, _1, _2, _3));
  registerStatusDatasetHandler("channels", bind(&FaceManager::listChannels, this, _1, _2, _3));
  registerStatusDatasetHandler("query", bind(&FaceManager::queryFaces, this, _1, _2, _3));

  // register notification stream
  m_postNotification = registerNotificationStream("events");
  m_faceAddConn = m_faceTable.afterAdd.connect([this] (const Face& face) {
    connectFaceStateChangeSignal(face);
    notifyFaceEvent(face, ndn::nfd::FACE_EVENT_CREATED);
  });
  m_faceRemoveConn = m_faceTable.beforeRemove.connect([this] (const Face& face) {
    notifyFaceEvent(face, ndn::nfd::FACE_EVENT_DESTROYED);
  });
}

void
FaceManager::createFace(const Name& topPrefix, const Interest& interest,
                        const ControlParameters& parameters,
                        const ndn::mgmt::CommandContinuation& done)
{
  FaceUri remoteUri;
  if (!remoteUri.parse(parameters.getUri())) {
    NFD_LOG_TRACE("failed to parse remote URI: " << parameters.getUri());
    done(ControlResponse(400, "Malformed command"));
    return;
  }

  if (!remoteUri.isCanonical()) {
    NFD_LOG_TRACE("received non-canonical remote URI: " << remoteUri.toString());
    done(ControlResponse(400, "Non-canonical remote URI"));
    return;
  }

  optional<FaceUri> localUri;
  if (parameters.hasLocalUri()) {
    localUri = FaceUri{};

    if (!localUri->parse(parameters.getLocalUri())) {
      NFD_LOG_TRACE("failed to parse local URI: " << parameters.getLocalUri());
      done(ControlResponse(400, "Malformed command"));
      return;
    }

    if (!localUri->isCanonical()) {
      NFD_LOG_TRACE("received non-canonical local URI: " << localUri->toString());
      done(ControlResponse(400, "Non-canonical local URI"));
      return;
    }
  }

  face::ProtocolFactory* factory = m_faceSystem.getFactoryByScheme(remoteUri.getScheme());
  if (factory == nullptr) {
    NFD_LOG_TRACE("received create request for unsupported protocol: " << remoteUri.getScheme());
    done(ControlResponse(406, "Unsupported protocol"));
    return;
  }

  face::FaceParams faceParams;
  faceParams.persistency = parameters.getFacePersistency();
  if (parameters.hasBaseCongestionMarkingInterval()) {
    faceParams.baseCongestionMarkingInterval = parameters.getBaseCongestionMarkingInterval();
  }
  if (parameters.hasDefaultCongestionThreshold()) {
    faceParams.defaultCongestionThreshold = parameters.getDefaultCongestionThreshold();
  }
  faceParams.wantLocalFields = parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
                               parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED);
  faceParams.wantLpReliability = parameters.hasFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED) &&
                                 parameters.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED);
  if (parameters.hasFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)) {
    faceParams.wantCongestionMarking = parameters.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED);
  }
  try {
    factory->createFace({remoteUri, localUri, faceParams},
                        bind(&FaceManager::afterCreateFaceSuccess, this, parameters, _1, done),
                        bind(&FaceManager::afterCreateFaceFailure, this, _1, _2, done));
  }
  catch (const std::runtime_error& error) {
    NFD_LOG_ERROR("Face creation failed: " << error.what());
    done(ControlResponse(500, "Face creation failed due to internal error"));
    return;
  }
  catch (const std::logic_error& error) {
    NFD_LOG_ERROR("Face creation failed: " << error.what());
    done(ControlResponse(500, "Face creation failed due to internal error"));
    return;
  }
}

void
FaceManager::afterCreateFaceSuccess(const ControlParameters& parameters,
                                    const shared_ptr<Face>& face,
                                    const ndn::mgmt::CommandContinuation& done)
{
  if (face->getId() != face::INVALID_FACEID) {// Face already exists
    NFD_LOG_TRACE("Attempted to create duplicate face of " << face->getId());

    ControlParameters response = collectFaceProperties(*face, true);
    done(ControlResponse(409, "Face with remote URI already exists").setBody(response.wireEncode()));
    return;
  }

  // If scope non-local and flags set to enable local fields, request shouldn't
  // have made it this far
  BOOST_ASSERT(face->getScope() == ndn::nfd::FACE_SCOPE_LOCAL ||
               !parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) ||
               (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
                !parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED)));

  m_faceTable.add(face);

  ControlParameters response = collectFaceProperties(*face, true);
  done(ControlResponse(200, "OK").setBody(response.wireEncode()));
}

void
FaceManager::afterCreateFaceFailure(uint32_t status,
                                    const std::string& reason,
                                    const ndn::mgmt::CommandContinuation& done)
{
  NFD_LOG_DEBUG("Face creation failed: " << reason);

  done(ControlResponse(status, reason));
}

void
FaceManager::updateFace(const Name& topPrefix, const Interest& interest,
                        const ControlParameters& parameters,
                        const ndn::mgmt::CommandContinuation& done)
{
  FaceId faceId = parameters.getFaceId();
  if (faceId == 0) {
    // Self-updating
    shared_ptr<lp::IncomingFaceIdTag> incomingFaceIdTag = interest.getTag<lp::IncomingFaceIdTag>();
    if (incomingFaceIdTag == nullptr) {
      NFD_LOG_TRACE("unable to determine face for self-update");
      done(ControlResponse(404, "No FaceId specified and IncomingFaceId not available"));
      return;
    }
    faceId = *incomingFaceIdTag;
  }

  Face* face = m_faceTable.get(faceId);

  if (face == nullptr) {
    NFD_LOG_TRACE("invalid face specified");
    done(ControlResponse(404, "Specified face does not exist"));
    return;
  }

  // Verify validity of requested changes
  ControlParameters response;
  bool areParamsValid = true;

  if (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      face->getScope() != ndn::nfd::FACE_SCOPE_LOCAL) {
    NFD_LOG_TRACE("received request to enable local fields on non-local face");
    areParamsValid = false;
    response.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED,
                        parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
  }

  // check whether the requested FacePersistency change is valid if it's present
  if (parameters.hasFacePersistency()) {
    auto persistency = parameters.getFacePersistency();
    if (!face->getTransport()->canChangePersistencyTo(persistency)) {
      NFD_LOG_TRACE("cannot change face persistency to " << persistency);
      areParamsValid = false;
      response.setFacePersistency(persistency);
    }
  }

  if (!areParamsValid) {
    done(ControlResponse(409, "Invalid properties specified").setBody(response.wireEncode()));
    return;
  }

  // All specified properties are valid, so make changes
  if (parameters.hasFacePersistency()) {
    face->setPersistency(parameters.getFacePersistency());
  }
  setLinkServiceOptions(*face, parameters);

  // Set ControlResponse fields
  response = collectFaceProperties(*face, false);

  done(ControlResponse(200, "OK").setBody(response.wireEncode()));
}

void
FaceManager::destroyFace(const Name& topPrefix, const Interest& interest,
                         const ControlParameters& parameters,
                         const ndn::mgmt::CommandContinuation& done)
{
  Face* face = m_faceTable.get(parameters.getFaceId());
  if (face != nullptr) {
    face->close();
  }

  done(ControlResponse(200, "OK").setBody(parameters.wireEncode()));
}

void
FaceManager::setLinkServiceOptions(Face& face,
                                   const ControlParameters& parameters)
{
  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  BOOST_ASSERT(linkService != nullptr);

  auto options = linkService->getOptions();
  if (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      face.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
    options.allowLocalFields = parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED);
  }
  if (parameters.hasFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED)) {
    options.reliabilityOptions.isEnabled = parameters.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED);
  }
  if (parameters.hasFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)) {
    options.allowCongestionMarking = parameters.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED);
  }
  if (parameters.hasBaseCongestionMarkingInterval()) {
    options.baseCongestionMarkingInterval = parameters.getBaseCongestionMarkingInterval();
  }
  if (parameters.hasDefaultCongestionThreshold()) {
    options.defaultCongestionThreshold = parameters.getDefaultCongestionThreshold();
  }
  linkService->setOptions(options);
}

ControlParameters
FaceManager::collectFaceProperties(const Face& face, bool wantUris)
{
  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  BOOST_ASSERT(linkService != nullptr);
  auto options = linkService->getOptions();

  ControlParameters params;
  params.setFaceId(face.getId())
        .setFacePersistency(face.getPersistency())
        .setBaseCongestionMarkingInterval(options.baseCongestionMarkingInterval)
        .setDefaultCongestionThreshold(options.defaultCongestionThreshold)
        .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, options.allowLocalFields, false)
        .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, options.reliabilityOptions.isEnabled, false)
        .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, options.allowCongestionMarking, false);
  if (wantUris) {
    params.setUri(face.getRemoteUri().toString())
          .setLocalUri(face.getLocalUri().toString());
  }
  return params;
}

void
FaceManager::listFaces(const Name& topPrefix, const Interest& interest,
                       ndn::mgmt::StatusDatasetContext& context)
{
  auto now = time::steady_clock::now();
  for (const Face& face : m_faceTable) {
    ndn::nfd::FaceStatus status = collectFaceStatus(face, now);
    context.append(status.wireEncode());
  }
  context.end();
}

void
FaceManager::listChannels(const Name& topPrefix, const Interest& interest,
                          ndn::mgmt::StatusDatasetContext& context)
{
  std::set<const face::ProtocolFactory*> factories = m_faceSystem.listProtocolFactories();
  for (const auto* factory : factories) {
    for (const auto& channel : factory->getChannels()) {
      ndn::nfd::ChannelStatus entry;
      entry.setLocalUri(channel->getUri().toString());
      context.append(entry.wireEncode());
    }
  }
  context.end();
}

void
FaceManager::queryFaces(const Name& topPrefix, const Interest& interest,
                        ndn::mgmt::StatusDatasetContext& context)
{
  ndn::nfd::FaceQueryFilter faceFilter;
  const Name& query = interest.getName();
  try {
    faceFilter.wireDecode(query[-1].blockFromValue());
  }
  catch (const tlv::Error& e) {
    NFD_LOG_DEBUG("Malformed query filter: " << e.what());
    return context.reject(ControlResponse(400, "Malformed filter"));
  }

  auto now = time::steady_clock::now();
  for (const Face& face : m_faceTable) {
    if (!matchFilter(faceFilter, face)) {
      continue;
    }
    ndn::nfd::FaceStatus status = collectFaceStatus(face, now);
    context.append(status.wireEncode());
  }

  context.end();
}

bool
FaceManager::matchFilter(const ndn::nfd::FaceQueryFilter& filter, const Face& face)
{
  if (filter.hasFaceId() &&
      filter.getFaceId() != static_cast<uint64_t>(face.getId())) {
    return false;
  }

  if (filter.hasUriScheme() &&
      filter.getUriScheme() != face.getRemoteUri().getScheme() &&
      filter.getUriScheme() != face.getLocalUri().getScheme()) {
    return false;
  }

  if (filter.hasRemoteUri() &&
      filter.getRemoteUri() != face.getRemoteUri().toString()) {
    return false;
  }

  if (filter.hasLocalUri() &&
      filter.getLocalUri() != face.getLocalUri().toString()) {
    return false;
  }

  if (filter.hasFaceScope() &&
      filter.getFaceScope() != face.getScope()) {
    return false;
  }

  if (filter.hasFacePersistency() &&
      filter.getFacePersistency() != face.getPersistency()) {
    return false;
  }

  if (filter.hasLinkType() &&
      filter.getLinkType() != face.getLinkType()) {
    return false;
  }

  return true;
}

ndn::nfd::FaceStatus
FaceManager::collectFaceStatus(const Face& face, const time::steady_clock::TimePoint& now)
{
  ndn::nfd::FaceStatus status;

  collectFaceProperties(face, status);

  time::steady_clock::TimePoint expirationTime = face.getExpirationTime();
  if (expirationTime != time::steady_clock::TimePoint::max()) {
    status.setExpirationPeriod(std::max(0_ms,
                                        time::duration_cast<time::milliseconds>(expirationTime - now)));
  }

  // Get LinkService options
  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  if (linkService != nullptr) {
    auto linkServiceOptions = linkService->getOptions();
    status.setBaseCongestionMarkingInterval(linkServiceOptions.baseCongestionMarkingInterval);
    status.setDefaultCongestionThreshold(linkServiceOptions.defaultCongestionThreshold);
  }

  const face::FaceCounters& counters = face.getCounters();
  status.setNInInterests(counters.nInInterests)
        .setNOutInterests(counters.nOutInterests)
        .setNInData(counters.nInData)
        .setNOutData(counters.nOutData)
        .setNInNacks(counters.nInNacks)
        .setNOutNacks(counters.nOutNacks)
        .setNInBytes(counters.nInBytes)
        .setNOutBytes(counters.nOutBytes);

  return status;
}

template<typename FaceTraits>
void
FaceManager::collectFaceProperties(const Face& face, FaceTraits& traits)
{
  traits.setFaceId(face.getId())
        .setRemoteUri(face.getRemoteUri().toString())
        .setLocalUri(face.getLocalUri().toString())
        .setFaceScope(face.getScope())
        .setFacePersistency(face.getPersistency())
        .setLinkType(face.getLinkType());

  // Set Flag bits
  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  if (linkService != nullptr) {
    auto linkServiceOptions = linkService->getOptions();
    traits.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, linkServiceOptions.allowLocalFields);
    traits.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED,
                      linkServiceOptions.reliabilityOptions.isEnabled);
    traits.setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED,
                      linkServiceOptions.allowCongestionMarking);
  }
}

void
FaceManager::notifyFaceEvent(const Face& face, ndn::nfd::FaceEventKind kind)
{
  ndn::nfd::FaceEventNotification notification;
  notification.setKind(kind);
  collectFaceProperties(face, notification);

  m_postNotification(notification.wireEncode());
}

void
FaceManager::connectFaceStateChangeSignal(const Face& face)
{
  using face::FaceState;

  FaceId faceId = face.getId();
  m_faceStateChangeConn[faceId] = face.afterStateChange.connect(
    [this, faceId, &face] (FaceState oldState, FaceState newState) {
      if (newState == FaceState::UP) {
        notifyFaceEvent(face, ndn::nfd::FACE_EVENT_UP);
      }
      else if (newState == FaceState::DOWN) {
        notifyFaceEvent(face, ndn::nfd::FACE_EVENT_DOWN);
      }
      else if (newState == FaceState::CLOSED) {
        // cannot use face.getId() because it may already be reset to INVALID_FACEID
        m_faceStateChangeConn.erase(faceId);
      }
    });
}

} // namespace nfd
