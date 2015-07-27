/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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
#include <ndn-cxx/management/nfd-face-status.hpp>

namespace nfd {
namespace rib {

NFD_LOG_INIT("RibManager");

const Name RibManager::COMMAND_PREFIX = "/localhost/nfd/rib";
const Name RibManager::REMOTE_COMMAND_PREFIX = "/localhop/nfd/rib";
const Name RibManager::FACES_LIST_DATASET_PREFIX = "/localhost/nfd/faces/list";

const size_t RibManager::COMMAND_UNSIGNED_NCOMPS =
  RibManager::COMMAND_PREFIX.size() +
  1 + // verb
  1;  // verb options

const size_t RibManager::COMMAND_SIGNED_NCOMPS =
  RibManager::COMMAND_UNSIGNED_NCOMPS +
  4; // (timestamp, nonce, signed info tlv, signature tlv)

const RibManager::SignedVerbAndProcessor RibManager::SIGNED_COMMAND_VERBS[] =
  {
    SignedVerbAndProcessor(
                           Name::Component("register"),
                           &RibManager::registerEntry
                           ),

    SignedVerbAndProcessor(
                           Name::Component("unregister"),
                           &RibManager::unregisterEntry
                           ),
  };

const RibManager::UnsignedVerbAndProcessor RibManager::UNSIGNED_COMMAND_VERBS[] =
  {
    UnsignedVerbAndProcessor(
                             Name::Component("list"),
                             &RibManager::listEntries
                             ),
  };

const Name RibManager::LIST_COMMAND_PREFIX("/localhost/nfd/rib/list");
const size_t RibManager::LIST_COMMAND_NCOMPS = LIST_COMMAND_PREFIX.size();

const time::seconds RibManager::ACTIVE_FACE_FETCH_INTERVAL = time::seconds(300);

RibManager::RibManager(ndn::Face& face, ndn::KeyChain& keyChain)
  : m_face(face)
  , m_keyChain(keyChain)
  , m_nfdController(m_face, m_keyChain)
  , m_localhostValidator(m_face)
  , m_localhopValidator(m_face)
  , m_faceMonitor(m_face)
  , m_isLocalhopEnabled(false)
  , m_remoteRegistrator(m_nfdController, m_keyChain, m_managedRib)
  , m_ribStatusPublisher(m_managedRib, face, LIST_COMMAND_PREFIX, m_keyChain)
  , m_fibUpdater(m_managedRib, m_nfdController)
  , m_signedVerbDispatch(SIGNED_COMMAND_VERBS,
                         SIGNED_COMMAND_VERBS +
                         (sizeof(SIGNED_COMMAND_VERBS) / sizeof(SignedVerbAndProcessor)))
  , m_unsignedVerbDispatch(UNSIGNED_COMMAND_VERBS,
                           UNSIGNED_COMMAND_VERBS +
                           (sizeof(UNSIGNED_COMMAND_VERBS) / sizeof(UnsignedVerbAndProcessor)))
{
}

RibManager::~RibManager()
{
  scheduler::cancel(m_activeFaceFetchEvent);
}

void
RibManager::startListening(const Name& commandPrefix, const ndn::OnInterest& onRequest)
{
  NFD_LOG_INFO("Listening on: " << commandPrefix);

  m_nfdController.start<ndn::nfd::FibAddNextHopCommand>(
    ControlParameters()
      .setName(commandPrefix)
      .setFaceId(0),
    bind(&RibManager::onNrdCommandPrefixAddNextHopSuccess, this, cref(commandPrefix), _1),
    bind(&RibManager::onNrdCommandPrefixAddNextHopError, this, cref(commandPrefix), _2));

  m_face.setInterestFilter(commandPrefix, onRequest);
}

void
RibManager::registerWithNfd()
{
  //check whether the components of localhop and localhost prefixes are same
  BOOST_ASSERT(COMMAND_PREFIX.size() == REMOTE_COMMAND_PREFIX.size());

  this->startListening(COMMAND_PREFIX, bind(&RibManager::onLocalhostRequest, this, _2));

  if (m_isLocalhopEnabled) {
    this->startListening(REMOTE_COMMAND_PREFIX,
                         bind(&RibManager::onLocalhopRequest, this, _2));
  }

  NFD_LOG_INFO("Start monitoring face create/destroy events");
  m_faceMonitor.onNotification.connect(bind(&RibManager::onNotification, this, _1));
  m_faceMonitor.start();

  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

void
RibManager::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("rib",
                               bind(&RibManager::onConfig, this, _1, _2, _3));
}

void
RibManager::onConfig(const ConfigSection& configSection,
                     bool isDryRun,
                     const std::string& filename)
{
  bool isRemoteRegisterEnabled = false;

  for (const auto& item : configSection) {
    if (item.first == "localhost_security") {
      m_localhostValidator.load(item.second, filename);
    }
    else if (item.first == "localhop_security") {
      m_localhopValidator.load(item.second, filename);
      m_isLocalhopEnabled = true;
    }
    else if (item.first == "remote_register") {
      m_remoteRegistrator.loadConfig(item.second);
      isRemoteRegisterEnabled = true;

      // Avoid other actions when isDryRun == true
      if (isDryRun) {
        continue;
      }

      m_remoteRegistrator.enable();
    }
    else {
      BOOST_THROW_EXCEPTION(Error("Unrecognized rib property: " + item.first));
    }
  }

  if (!isRemoteRegisterEnabled) {
    m_remoteRegistrator.disable();
  }
}

void
RibManager::sendResponse(const Name& name,
                         const ControlResponse& response)
{
  const Block& encodedControl = response.wireEncode();

  shared_ptr<Data> responseData = make_shared<Data>(name);
  responseData->setContent(encodedControl);

  m_keyChain.sign(*responseData);
  m_face.put(*responseData);
}

void
RibManager::sendResponse(const Name& name,
                         uint32_t code,
                         const std::string& text)
{
  ControlResponse response(code, text);
  sendResponse(name, response);
}

void
RibManager::onLocalhostRequest(const Interest& request)
{
  const Name& command = request.getName();
  const Name::Component& verb = command.get(COMMAND_PREFIX.size());

  UnsignedVerbDispatchTable::const_iterator unsignedVerbProcessor = m_unsignedVerbDispatch.find(verb);

  if (unsignedVerbProcessor != m_unsignedVerbDispatch.end()) {
    NFD_LOG_DEBUG("command result: processing unsigned verb: " << verb);
    (unsignedVerbProcessor->second)(this, request);
  }
  else {
    m_localhostValidator.validate(request,
                                  bind(&RibManager::onCommandValidated, this, _1),
                                  bind(&RibManager::onCommandValidationFailed, this, _1, _2));
  }
}

void
RibManager::onLocalhopRequest(const Interest& request)
{
  m_localhopValidator.validate(request,
                               bind(&RibManager::onCommandValidated, this, _1),
                               bind(&RibManager::onCommandValidationFailed, this, _1, _2));
}

void
RibManager::onCommandValidated(const shared_ptr<const Interest>& request)
{
  // REMOTE_COMMAND_PREFIX number of componenets are same as
  // NRD_COMMAND_PREFIX's so no extra checks are required.

  const Name& command = request->getName();
  const Name::Component& verb = command[COMMAND_PREFIX.size()];
  const Name::Component& parameterComponent = command[COMMAND_PREFIX.size() + 1];

  SignedVerbDispatchTable::const_iterator verbProcessor = m_signedVerbDispatch.find(verb);

  if (verbProcessor != m_signedVerbDispatch.end()) {

    ControlParameters parameters;
    if (!extractParameters(parameterComponent, parameters)) {
      NFD_LOG_DEBUG("command result: malformed verb: " << verb);

      if (static_cast<bool>(request)) {
        sendResponse(command, 400, "Malformed command");
      }

      return;
    }

    NFD_LOG_DEBUG("command result: processing verb: " << verb);
    (verbProcessor->second)(this, request, parameters);
  }
  else {
    NFD_LOG_DEBUG("Unsupported command: " << verb);

    if (static_cast<bool>(request)) {
      sendResponse(request->getName(), 501, "Unsupported command");
    }
  }
}

void
RibManager::registerEntry(const shared_ptr<const Interest>& request,
                          ControlParameters& parameters)
{
  ndn::nfd::RibRegisterCommand command;

  if (!validateParameters(command, parameters)) {
    NFD_LOG_DEBUG("register result: FAIL reason: malformed");

    if (static_cast<bool>(request)) {
      sendResponse(request->getName(), 400, "Malformed command");
    }

    return;
  }

  bool isSelfRegistration = (!parameters.hasFaceId() || parameters.getFaceId() == 0);
  if (isSelfRegistration) {
    parameters.setFaceId(request->getIncomingFaceId());
  }

  // Respond since command is valid and authorized
  sendSuccessResponse(request, parameters);

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
      bind(&Rib::onRouteExpiration, &m_managedRib, parameters.getName(), route));

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

  m_managedRib.beginApplyUpdate(update,
                                bind(&RibManager::onRibUpdateSuccess, this, update),
                                bind(&RibManager::onRibUpdateFailure, this, update, _1, _2));

  m_registeredFaces.insert(route.faceId);
}

void
RibManager::unregisterEntry(const shared_ptr<const Interest>& request,
                            ControlParameters& params)
{
  ndn::nfd::RibUnregisterCommand command;

  // Passing all parameters gives error in validation,
  // so passing only the required arguments.
  ControlParameters parameters;
  parameters.setName(params.getName());

  if (params.hasFaceId()) {
    parameters.setFaceId(params.getFaceId());
  }

  if (params.hasOrigin()) {
    parameters.setOrigin(params.getOrigin());
  }

  if (!validateParameters(command, parameters)) {
    NFD_LOG_DEBUG("unregister result: FAIL reason: malformed");

    if (static_cast<bool>(request)) {
      sendResponse(request->getName(), 400, "Malformed command");
    }

    return;
  }

  bool isSelfRegistration = (!parameters.hasFaceId() || parameters.getFaceId() == 0);
  if (isSelfRegistration) {
    parameters.setFaceId(request->getIncomingFaceId());
  }

  // Respond since command is valid and authorized
  sendSuccessResponse(request, parameters);

  Route route;
  route.faceId = parameters.getFaceId();
  route.origin = parameters.getOrigin();

  NFD_LOG_INFO("Removing route " << parameters.getName() << " nexthop=" << route.faceId
                                                         << " origin=" << route.origin);

  RibUpdate update;
  update.setAction(RibUpdate::UNREGISTER)
        .setName(parameters.getName())
        .setRoute(route);

  m_managedRib.beginApplyUpdate(update,
                                bind(&RibManager::onRibUpdateSuccess, this, update),
                                bind(&RibManager::onRibUpdateFailure, this, update, _1, _2));
}

void
RibManager::onCommandValidationFailed(const shared_ptr<const Interest>& request,
                                      const std::string& failureInfo)
{
  NFD_LOG_DEBUG("RibRequestValidationFailed: " << failureInfo);

  if (static_cast<bool>(request)) {
    sendResponse(request->getName(), 403, failureInfo);
  }
}


bool
RibManager::extractParameters(const Name::Component& parameterComponent,
                              ControlParameters& extractedParameters)
{
  try {
    Block rawParameters = parameterComponent.blockFromValue();
    extractedParameters.wireDecode(rawParameters);
  }
  catch (const tlv::Error&) {
    return false;
  }

  NFD_LOG_DEBUG("Parameters parsed OK");
  return true;
}

bool
RibManager::validateParameters(const ControlCommand& command,
                               ControlParameters& parameters)
{
  try {
    command.validateRequest(parameters);
  }
  catch (const ControlCommand::ArgumentError&) {
    return false;
  }

  command.applyDefaultsToRequest(parameters);

  return true;
}

void
RibManager::onCommandError(uint32_t code, const std::string& error,
                           const shared_ptr<const Interest>& request,
                           const Route& route)
{
  NFD_LOG_ERROR("NFD returned an error: " << error << " (code: " << code << ")");

  ControlResponse response;

  if (code == 404) {
    response.setCode(code);
    response.setText(error);
  }
  else {
    response.setCode(533);
    std::ostringstream os;
    os << "Failure to update NFD " << "(NFD Error: " << code << " " << error << ")";
    response.setText(os.str());
  }

  if (static_cast<bool>(request)) {
    sendResponse(request->getName(), response);
  }
}

void
RibManager::onRegSuccess(const shared_ptr<const Interest>& request,
                         const ControlParameters& parameters,
                         const Route& route)
{
  ControlResponse response;

  response.setCode(200);
  response.setText("Success");
  response.setBody(parameters.wireEncode());

  NFD_LOG_TRACE("onRegSuccess: registered " << route);

  if (static_cast<bool>(request)) {
    sendResponse(request->getName(), response);
  }
}


void
RibManager::onUnRegSuccess(const shared_ptr<const Interest>& request,
                           const ControlParameters& parameters,
                           const Route& route)
{
  ControlResponse response;

  response.setCode(200);
  response.setText("Success");
  response.setBody(parameters.wireEncode());

  NFD_LOG_TRACE("onUnRegSuccess: unregistered " << route);

  if (static_cast<bool>(request)) {
    sendResponse(request->getName(), response);
  }
}

void
RibManager::sendSuccessResponse(const shared_ptr<const Interest>& request,
                                const ControlParameters& parameters)
{
  if (!static_cast<bool>(request)) {
    return;
  }

  ControlResponse response;

  response.setCode(200);
  response.setText("Success");
  response.setBody(parameters.wireEncode());

  if (static_cast<bool>(request)) {
    sendResponse(request->getName(), response);
  }
}

void
RibManager::sendErrorResponse(uint32_t code, const std::string& error,
                              const shared_ptr<const Interest>& request)
{
  NFD_LOG_ERROR("NFD returned an error: " << error << " (code: " << code << ")");

  if (!static_cast<bool>(request)) {
    return;
  }

  ControlResponse response;

  if (code == 404) {
    response.setCode(code);
    response.setText(error);
  }
  else {
    response.setCode(533);
    std::ostringstream os;
    os << "Failure to update NFD " << "(NFD Error: " << code << " " << error << ")";
    response.setText(os.str());
  }

  if (static_cast<bool>(request)) {
    sendResponse(request->getName(), response);
  }
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
RibManager::onNrdCommandPrefixAddNextHopSuccess(const Name& prefix,
                                                const ndn::nfd::ControlParameters& result)
{
  NFD_LOG_DEBUG("Successfully registered " + prefix.toUri() + " with NFD");

  // Routes must be inserted into the RIB so route flags can be applied
  Route route;
  route.faceId = result.getFaceId();
  route.origin = ndn::nfd::ROUTE_ORIGIN_APP;
  route.expires = time::steady_clock::TimePoint::max();
  route.flags = ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;

  m_managedRib.insert(prefix, route);

  m_registeredFaces.insert(route.faceId);
}

void
RibManager::onNrdCommandPrefixAddNextHopError(const Name& name, const std::string& msg)
{
  BOOST_THROW_EXCEPTION(Error("Error in setting interest filter (" + name.toUri() + "): " + msg));
}

void
RibManager::onControlHeaderSuccess()
{
  NFD_LOG_DEBUG("Local control header enabled");
}

void
RibManager::onControlHeaderError(uint32_t code, const std::string& reason)
{
  std::ostringstream os;
  os << "Couldn't enable local control header "
     << "(code: " << code << ", info: " << reason << ")";
  BOOST_THROW_EXCEPTION(Error(os.str()));
}

void
RibManager::enableLocalControlHeader()
{
  m_nfdController.start<ndn::nfd::FaceEnableLocalControlCommand>(
    ControlParameters()
      .setLocalControlFeature(ndn::nfd::LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID),
    bind(&RibManager::onControlHeaderSuccess, this),
    bind(&RibManager::onControlHeaderError, this, _1, _2));
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
RibManager::onFaceDestroyedEvent(uint64_t faceId)
{
  m_managedRib.beginRemoveFace(faceId);
  m_registeredFaces.erase(faceId);
}

void
RibManager::listEntries(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (commandNComps < LIST_COMMAND_NCOMPS || !LIST_COMMAND_PREFIX.isPrefixOf(command)) {
    NFD_LOG_DEBUG("command result: malformed");

    sendResponse(command, 400, "Malformed command");
    return;
  }

  m_ribStatusPublisher.publish();
}

void
RibManager::scheduleActiveFaceFetch(const time::seconds& timeToWait)
{
  scheduler::cancel(m_activeFaceFetchEvent);

  m_activeFaceFetchEvent = scheduler::schedule(timeToWait,
                                               bind(&RibManager::fetchActiveFaces, this));
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
  for (uint64_t faceId : m_registeredFaces) {
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
RibManager::onFetchFaceStatusTimeout()
{
  std::cerr << "Face Status Dataset request timed out" << std::endl;
  scheduleActiveFaceFetch(ACTIVE_FACE_FETCH_INTERVAL);
}

} // namespace rib
} // namespace nfd
