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

#include "face-manager.hpp"
#include "face/generic-link-service.hpp"
#include "face/protocol-factory.hpp"
#include "fw/face-table.hpp"

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/channel-status.hpp>

namespace nfd {

NFD_LOG_INIT("FaceManager");

FaceManager::FaceManager(FaceSystem& faceSystem,
                         Dispatcher& dispatcher, CommandAuthenticator& authenticator)
  : NfdManagerBase(dispatcher, authenticator, "faces")
  , m_faceSystem(faceSystem)
  , m_faceTable(faceSystem.getFaceTable())
{
  registerCommandHandler<ndn::nfd::FaceCreateCommand>("create",
    bind(&FaceManager::createFace, this, _2, _3, _4, _5));

  registerCommandHandler<ndn::nfd::FaceUpdateCommand>("update",
    bind(&FaceManager::updateFace, this, _2, _3, _4, _5));

  registerCommandHandler<ndn::nfd::FaceDestroyCommand>("destroy",
    bind(&FaceManager::destroyFace, this, _2, _3, _4, _5));

  registerCommandHandler<ndn::nfd::FaceEnableLocalControlCommand>("enable-local-control",
    bind(&FaceManager::enableLocalControl, this, _2, _3, _4, _5));

  registerCommandHandler<ndn::nfd::FaceDisableLocalControlCommand>("disable-local-control",
    bind(&FaceManager::disableLocalControl, this, _2, _3, _4, _5));

  registerStatusDatasetHandler("list", bind(&FaceManager::listFaces, this, _1, _2, _3));
  registerStatusDatasetHandler("channels", bind(&FaceManager::listChannels, this, _1, _2, _3));
  registerStatusDatasetHandler("query", bind(&FaceManager::queryFaces, this, _1, _2, _3));

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
FaceManager::setConfigFile(ConfigFile& configFile)
{
  m_faceSystem.setConfigFile(configFile);
}

void
FaceManager::createFace(const Name& topPrefix, const Interest& interest,
                        const ControlParameters& parameters,
                        const ndn::mgmt::CommandContinuation& done)
{
  FaceUri uri;
  if (!uri.parse(parameters.getUri())) {
    NFD_LOG_TRACE("failed to parse URI");
    done(ControlResponse(400, "Malformed command"));
    return;
  }

  if (!uri.isCanonical()) {
    NFD_LOG_TRACE("received non-canonical URI");
    done(ControlResponse(400, "Non-canonical URI"));
    return;
  }

  ProtocolFactory* factory = m_faceSystem.getFactoryByScheme(uri.getScheme());
  if (factory == nullptr) {
    NFD_LOG_TRACE("received create request for unsupported protocol");
    done(ControlResponse(406, "Unsupported protocol"));
    return;
  }

  try {
    factory->createFace(uri,
      parameters.getFacePersistency(),
      parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
        parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED),
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

/**
 * \todo #3232
 *       If the creation of this face would conflict with an existing face (e.g. same underlying
 *       protocol and remote address, or a NIC-associated permanent face), the command will fail
 *       with StatusCode 409.
 */
void
FaceManager::afterCreateFaceSuccess(const ControlParameters& parameters,
                                    const shared_ptr<Face>& face,
                                    const ndn::mgmt::CommandContinuation& done)
{
  // TODO: Re-enable check in #3232
  //if (face->getId() != face::INVALID_FACEID) {
  //// Face already exists
  //ControlParameters response;
  //response.setFaceId(face->getId());
  //response.setUri(face->getRemoteUri().toString());
  //response.setFacePersistency(face->getPersistency());
  //
  //auto linkService = dynamic_cast<face::GenericLinkService*>(face->getLinkService());
  //BOOST_ASSERT(linkService != nullptr);
  //auto options = linkService->getOptions();
  //response.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, options.allowLocalFields, false);
  //
  //  NFD_LOG_TRACE("Attempted to create duplicate face of " << face->getId());
  //  done(ControlResponse(409, "Face with remote URI already exists").setBody(response.wireEncode()));
  //}
  //else {
  // If scope non-local and flags set to enable local fields, request shouldn't
  // have made it this far
  BOOST_ASSERT(face->getScope() == ndn::nfd::FACE_SCOPE_LOCAL ||
               !parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) ||
               (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
                !parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED)));

  m_faceTable.add(face);

  ControlParameters response;
  response.setFaceId(face->getId());
  response.setFacePersistency(face->getPersistency());
  response.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED,
                      parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) ?
                        parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) : false,
                      false);

  done(ControlResponse(200, "OK").setBody(response.wireEncode()));
  //}
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

  if (parameters.hasFacePersistency()) {
    // TODO #3232: Add FacePersistency updating
    NFD_LOG_TRACE("received unsupported face persistency change");
    areParamsValid = false;
    response.setFacePersistency(parameters.getFacePersistency());
  }

  if (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      face->getScope() != ndn::nfd::FACE_SCOPE_LOCAL) {
    NFD_LOG_TRACE("received request to enable local fields on non-local face");
    areParamsValid = false;
    response.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED,
                        parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
  }

  if (!areParamsValid) {
    done(ControlResponse(409, "Invalid properties specified").setBody(response.wireEncode()));
    return;
  }

  // All specified properties are valid, so make changes

  // TODO #3232: Add FacePersistency updating

  setLinkServiceOptions(*face, parameters, response);

  // Set ControlResponse fields
  response.setFaceId(faceId);
  response.setFacePersistency(face->getPersistency());
  response.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED,
                      parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) ?
                        parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) : false,
                      false);

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
FaceManager::enableLocalControl(const Name& topPrefix, const Interest& interest,
                                const ControlParameters& parameters,
                                const ndn::mgmt::CommandContinuation& done)
{
  Face* face = findFaceForLocalControl(interest, parameters, done);
  if (!face) {
    return;
  }

  // enable-local-control will enable all local fields in GenericLinkService
  auto service = dynamic_cast<face::GenericLinkService*>(face->getLinkService());
  if (service == nullptr) {
    return done(ControlResponse(503, "LinkService type not supported"));
  }

  face::GenericLinkService::Options options = service->getOptions();
  options.allowLocalFields = true;
  service->setOptions(options);

  return done(ControlResponse(200, "OK: enable all local fields on GenericLinkService")
              .setBody(parameters.wireEncode()));
}

void
FaceManager::disableLocalControl(const Name& topPrefix, const Interest& interest,
                                 const ControlParameters& parameters,
                                 const ndn::mgmt::CommandContinuation& done)
{
  Face* face = findFaceForLocalControl(interest, parameters, done);
  if (!face) {
    return;
  }

  // disable-local-control will disable all local fields in GenericLinkService
  auto service = dynamic_cast<face::GenericLinkService*>(face->getLinkService());
  if (service == nullptr) {
    return done(ControlResponse(503, "LinkService type not supported"));
  }

  face::GenericLinkService::Options options = service->getOptions();
  options.allowLocalFields = false;
  service->setOptions(options);

  return done(ControlResponse(200, "OK: disable all local fields on GenericLinkService")
              .setBody(parameters.wireEncode()));
}

Face*
FaceManager::findFaceForLocalControl(const Interest& request,
                                     const ControlParameters& parameters,
                                     const ndn::mgmt::CommandContinuation& done)
{
  shared_ptr<lp::IncomingFaceIdTag> incomingFaceIdTag = request.getTag<lp::IncomingFaceIdTag>();
  // NDNLPv2 says "application MUST be prepared to receive a packet without IncomingFaceId field",
  // but it's fine to assert IncomingFaceId is available, because InternalFace lives inside NFD
  // and is initialized synchronously with IncomingFaceId field enabled.
  BOOST_ASSERT(incomingFaceIdTag != nullptr);

  Face* face = m_faceTable.get(*incomingFaceIdTag);
  if (face == nullptr) {
    NFD_LOG_DEBUG("FaceId " << *incomingFaceIdTag << " not found");
    done(ControlResponse(410, "Face not found"));
    return nullptr;
  }

  if (face->getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL) {
    NFD_LOG_DEBUG("Cannot enable local control on non-local FaceId " << face->getId());
    done(ControlResponse(412, "Face is non-local"));
    return nullptr;
  }

  return face;
}

void
FaceManager::setLinkServiceOptions(Face& face,
                                   const ControlParameters& parameters,
                                   ControlParameters& response)
{
  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  BOOST_ASSERT(linkService != nullptr);

  auto options = linkService->getOptions();
  if (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      face.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
    options.allowLocalFields = parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED);
  }
  linkService->setOptions(options);

  // Set Flags for ControlResponse
  response.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, options.allowLocalFields, false);
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
  std::set<const ProtocolFactory*> factories = m_faceSystem.listProtocolFactories();
  for (const ProtocolFactory* factory : factories) {
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
    status.setExpirationPeriod(std::max(time::milliseconds(0),
                                        time::duration_cast<time::milliseconds>(expirationTime - now)));
  }

  const face::FaceCounters& counters = face.getCounters();
  status.setNInInterests(counters.nInInterests)
        .setNOutInterests(counters.nOutInterests)
        .setNInDatas(counters.nInData)
        .setNOutDatas(counters.nOutData)
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
  FaceId faceId = face.getId();
  m_faceStateChangeConn[faceId] = face.afterStateChange.connect(
    [this, faceId] (face::FaceState oldState, face::FaceState newState) {
      const Face& face = *m_faceTable.get(faceId);

      if (newState == face::FaceState::UP) {
        notifyFaceEvent(face, ndn::nfd::FACE_EVENT_UP);
      }
      else if (newState == face::FaceState::DOWN) {
        notifyFaceEvent(face, ndn::nfd::FACE_EVENT_DOWN);
      }
      else if (newState == face::FaceState::CLOSED) {
        m_faceStateChangeConn.erase(faceId);
      }
    });
}

} // namespace nfd
