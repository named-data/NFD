/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

RibManager::RibManager(ndn::Face& face)
  : m_face(face)
  , m_nfdController(m_face, m_keyChain)
  , m_localhostValidator(m_face)
  , m_localhopValidator(m_face)
  , m_faceMonitor(m_face)
  , m_isLocalhopEnabled(false)
  , m_remoteRegistrator(m_nfdController, m_keyChain, m_managedRib)
  , m_ribStatusPublisher(m_managedRib, face, LIST_COMMAND_PREFIX, m_keyChain)
  , m_lastTransactionId(0)
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
    bind(&RibManager::onNrdCommandPrefixAddNextHopSuccess, this, cref(commandPrefix)),
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
  m_faceMonitor.onNotification += bind(&RibManager::onNotification, this, _1);
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
  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end(); ++i)
    {
      if (i->first == "localhost_security")
          m_localhostValidator.load(i->second, filename);
      else if (i->first == "localhop_security")
        {
          m_localhopValidator.load(i->second, filename);
          m_isLocalhopEnabled = true;
        }
      else if (i->first == "remote_register")
        {
          m_remoteRegistrator.loadConfig(i->second);

          // register callback to the RIB.
          // do remote registration after an entry is inserted into the RIB.
          // do remote unregistration after an entry is erased from the RIB.
          m_managedRib.afterInsertEntry += [this] (const Name& prefix) {
            m_remoteRegistrator.registerPrefix(prefix);
          };

          m_managedRib.afterEraseEntry += [this] (const Name& prefix) {
            m_remoteRegistrator.unregisterPrefix(prefix);
          };
        }
      else
        throw Error("Unrecognized rib property: " + i->first);
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

  if (unsignedVerbProcessor != m_unsignedVerbDispatch.end())
    {
      NFD_LOG_DEBUG("command result: processing unsigned verb: " << verb);
      (unsignedVerbProcessor->second)(this, request);
    }
  else
    {
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
  if (verbProcessor != m_signedVerbDispatch.end())
    {
      ControlParameters parameters;
      if (!extractParameters(parameterComponent, parameters))
        {
          NFD_LOG_DEBUG("command result: malformed verb: " << verb);
          if (static_cast<bool>(request))
            sendResponse(command, 400, "Malformed command");
          return;
        }

      NFD_LOG_DEBUG("command result: processing verb: " << verb);
      (verbProcessor->second)(this, request, parameters);
    }
  else
    {
      NFD_LOG_DEBUG("Unsupported command: " << verb);
      if (static_cast<bool>(request))
        sendResponse(request->getName(), 501, "Unsupported command");
    }
}

void
RibManager::registerEntry(const shared_ptr<const Interest>& request,
                          ControlParameters& parameters)
{
  ndn::nfd::RibRegisterCommand command;

  if (!validateParameters(command, parameters))
    {
      NFD_LOG_DEBUG("register result: FAIL reason: malformed");

      if (static_cast<bool>(request))
        {
          sendResponse(request->getName(), 400, "Malformed command");
        }

      return;
    }

  bool isSelfRegistration = (!parameters.hasFaceId() || parameters.getFaceId() == 0);
  if (isSelfRegistration)
    {
      parameters.setFaceId(request->getIncomingFaceId());
    }

  FaceEntry faceEntry;
  faceEntry.faceId = parameters.getFaceId();
  faceEntry.origin = parameters.getOrigin();
  faceEntry.cost = parameters.getCost();
  faceEntry.flags = parameters.getFlags();

  if (parameters.hasExpirationPeriod() &&
      parameters.getExpirationPeriod() != time::milliseconds::max())
    {
      faceEntry.expires = time::steady_clock::now() + parameters.getExpirationPeriod();

      // Schedule a new event, the old one will be cancelled during rib insertion.
      EventId eventId;
      eventId = scheduler::schedule(parameters.getExpirationPeriod(),
                                    bind(&RibManager::expireEntry,
                                    this, shared_ptr<Interest>(), parameters));
      NFD_LOG_TRACE("Scheduled unregistration at: " << faceEntry.expires <<
                    " with EventId: " << eventId);

      //set the  NewEventId of this entry
      faceEntry.setExpirationEvent(eventId);
    }
  else
    {
      faceEntry.expires = time::steady_clock::TimePoint::max();
    }

  NFD_LOG_TRACE("register prefix: " << faceEntry);

  m_managedRib.insert(parameters.getName(), faceEntry);
  m_registeredFaces.insert(faceEntry.faceId);

  sendUpdatesToFib(request, parameters);
}

void
RibManager::expireEntry(const shared_ptr<const Interest>& request, ControlParameters& params)
{
  FaceEntry face;
  face.faceId = params.getFaceId();
  face.origin = params.getOrigin();
  face.cost = params.getCost();
  face.flags = params.getFlags();

  NFD_LOG_DEBUG(face << " for " << params.getName() << " has expired");
  unregisterEntry(request, params);
}

void
RibManager::unregisterEntry(const shared_ptr<const Interest>& request,
                            ControlParameters& params)
{
  ndn::nfd::RibUnregisterCommand command;

  //passing all parameters gives error in validation.
  //so passing only the required arguments.
  ControlParameters parameters;
  parameters.setName(params.getName());
  if (params.hasFaceId())
    parameters.setFaceId(params.getFaceId());
  if (params.hasOrigin())
    parameters.setOrigin(params.getOrigin());

  if (!validateParameters(command, parameters))
    {
      NFD_LOG_DEBUG("unregister result: FAIL reason: malformed");
      if (static_cast<bool>(request))
        sendResponse(request->getName(), 400, "Malformed command");
      return;
    }

  bool isSelfRegistration = (!parameters.hasFaceId() || parameters.getFaceId() == 0);
  if (isSelfRegistration)
    {
      parameters.setFaceId(request->getIncomingFaceId());
    }

  FaceEntry faceEntry;
  faceEntry.faceId = parameters.getFaceId();
  faceEntry.origin = parameters.getOrigin();

  NFD_LOG_TRACE("unregister prefix: " << faceEntry);

  m_managedRib.erase(parameters.getName(), faceEntry);

  sendUpdatesToFib(request, parameters);
}

void
RibManager::onCommandValidationFailed(const shared_ptr<const Interest>& request,
                                      const std::string& failureInfo)
{
  NFD_LOG_DEBUG("RibRequestValidationFailed: " << failureInfo);
  if (static_cast<bool>(request))
    sendResponse(request->getName(), 403, failureInfo);
}


bool
RibManager::extractParameters(const Name::Component& parameterComponent,
                              ControlParameters& extractedParameters)
{
  try
    {
      Block rawParameters = parameterComponent.blockFromValue();
      extractedParameters.wireDecode(rawParameters);
    }
  catch (const tlv::Error&)
    {
      return false;
    }

  NFD_LOG_DEBUG("Parameters parsed OK");
  return true;
}

bool
RibManager::validateParameters(const ControlCommand& command,
                               ControlParameters& parameters)
{
  try
    {
      command.validateRequest(parameters);
    }
  catch (const ControlCommand::ArgumentError&)
    {
      return false;
    }

  command.applyDefaultsToRequest(parameters);

  return true;
}

void
RibManager::onCommandError(uint32_t code, const std::string& error,
                           const shared_ptr<const Interest>& request,
                           const FaceEntry& faceEntry)
{
  NFD_LOG_ERROR("NFD returned an error: " << error << " (code: " << code << ")");

  ControlResponse response;

  if (code == 404)
    {
      response.setCode(code);
      response.setText(error);
    }
  else
    {
      response.setCode(533);
      std::ostringstream os;
      os << "Failure to update NFD " << "(NFD Error: " << code << " " << error << ")";
      response.setText(os.str());
    }

  if (static_cast<bool>(request))
    sendResponse(request->getName(), response);
}

void
RibManager::onRegSuccess(const shared_ptr<const Interest>& request,
                         const ControlParameters& parameters,
                         const FaceEntry& faceEntry)
{
  ControlResponse response;

  response.setCode(200);
  response.setText("Success");
  response.setBody(parameters.wireEncode());

  NFD_LOG_TRACE("onRegSuccess: registered " << faceEntry);

  if (static_cast<bool>(request))
    sendResponse(request->getName(), response);
}


void
RibManager::onUnRegSuccess(const shared_ptr<const Interest>& request,
                           const ControlParameters& parameters,
                           const FaceEntry& faceEntry)
{
  ControlResponse response;

  response.setCode(200);
  response.setText("Success");
  response.setBody(parameters.wireEncode());

  NFD_LOG_TRACE("onUnRegSuccess: unregistered " << faceEntry);

  if (static_cast<bool>(request))
    sendResponse(request->getName(), response);
}

void
RibManager::sendSuccessResponse(const shared_ptr<const Interest>& request,
                                const ControlParameters& parameters)
{
  if (!static_cast<bool>(request))
    {
      return;
    }

  ControlResponse response;

  response.setCode(200);
  response.setText("Success");
  response.setBody(parameters.wireEncode());

  if (static_cast<bool>(request))
    sendResponse(request->getName(), response);
}

void
RibManager::sendErrorResponse(uint32_t code, const std::string& error,
                              const shared_ptr<const Interest>& request)
{
  NFD_LOG_ERROR("NFD returned an error: " << error << " (code: " << code << ")");

  if (!static_cast<bool>(request))
    {
      return;
    }

  ControlResponse response;

  if (code == 404)
    {
      response.setCode(code);
      response.setText(error);
    }
  else
    {
      response.setCode(533);
      std::ostringstream os;
      os << "Failure to update NFD " << "(NFD Error: " << code << " " << error << ")";
      response.setText(os.str());
    }

  if (static_cast<bool>(request))
    sendResponse(request->getName(), response);
}

void
RibManager::onNrdCommandPrefixAddNextHopSuccess(const Name& prefix)
{
  NFD_LOG_DEBUG("Successfully registered " + prefix.toUri() + " with NFD");
}

void
RibManager::onNrdCommandPrefixAddNextHopError(const Name& name, const std::string& msg)
{
  throw Error("Error in setting interest filter (" + name.toUri() + "): " + msg);
}

bool
RibManager::isTransactionComplete(const TransactionId transactionId)
{
  FibTransactionTable::iterator it = m_pendingFibTransactions.find(transactionId);

  if (it != m_pendingFibTransactions.end())
    {
      int& updatesLeft = it->second;

      updatesLeft--;

      // All of the updates have been applied successfully
      if (updatesLeft == 0)
        {
          m_pendingFibTransactions.erase(it);
          return true;
        }
    }

    return false;
}

void
RibManager::invalidateTransaction(const TransactionId transactionId)
{
  FibTransactionTable::iterator it = m_pendingFibTransactions.find(transactionId);

  if (it != m_pendingFibTransactions.end())
    {
      m_pendingFibTransactions.erase(it);
    }
}

void
RibManager::onAddNextHopSuccess(const shared_ptr<const Interest>& request,
                                const ControlParameters& parameters,
                                const TransactionId transactionId,
                                const bool shouldSendResponse)
{
  if (isTransactionComplete(transactionId) && shouldSendResponse)
    {
      sendSuccessResponse(request, parameters);
    }
}

void
RibManager::onAddNextHopError(uint32_t code, const std::string& error,
                              const shared_ptr<const Interest>& request,
                              const TransactionId transactionId, const bool shouldSendResponse)
{
  invalidateTransaction(transactionId);

  if (shouldSendResponse)
  {
    sendErrorResponse(code, error, request);
  }

  // Since the FIB rejected the update, clean up the invalid face
  scheduleActiveFaceFetch(time::seconds(1));
}

void
RibManager::onRemoveNextHopSuccess(const shared_ptr<const Interest>& request,
                                   const ControlParameters& parameters,
                                   const TransactionId transactionId,
                                   const bool shouldSendResponse)
{
  if (isTransactionComplete(transactionId) && shouldSendResponse)
    {
      sendSuccessResponse(request, parameters);
    }
}

void
RibManager::onRemoveNextHopError(uint32_t code, const std::string& error,
                                 const shared_ptr<const Interest>& request,
                                 const TransactionId transactionId, const bool shouldSendResponse)
{
  invalidateTransaction(transactionId);

  if (shouldSendResponse)
  {
    sendErrorResponse(code, error, request);
  }
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
  throw Error(os.str());
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

  if (notification.getKind() == ndn::nfd::FACE_EVENT_DESTROYED)
    {
      NFD_LOG_DEBUG("Received notification for destroyed faceId: " << notification.getFaceId());

      scheduler::schedule(time::seconds(0),
                          bind(&RibManager::processErasureAfterNotification, this,
                               notification.getFaceId()));
    }
}

void
RibManager::processErasureAfterNotification(uint64_t faceId)
{
  m_managedRib.erase(faceId);
  m_registeredFaces.erase(faceId);

  sendUpdatesToFibAfterFaceDestroyEvent();
}

void
RibManager::sendUpdatesToFib(const shared_ptr<const Interest>& request,
                             const ControlParameters& parameters)
{
  const Rib::FibUpdateList& updates = m_managedRib.getFibUpdates();

  // If no updates were generated, consider the operation a success
  if (updates.empty())
    {
      sendSuccessResponse(request, parameters);
      return;
    }

  bool shouldWaitToRespond = false;

  // An application request should wait for all FIB updates to be applied
  // successfully before sending a response
  if (parameters.getOrigin() == ndn::nfd::ROUTE_ORIGIN_APP)
    {
      shouldWaitToRespond = true;
    }
  else // Respond immediately
    {
      sendSuccessResponse(request, parameters);
    }

  std::string updateString = (updates.size() == 1) ? " update" : " updates";
  NFD_LOG_DEBUG("Applying " << updates.size() << updateString << " to FIB");

  // Assign an ID to this FIB transaction
  TransactionId currentTransactionId = ++m_lastTransactionId;

  // Add this transaction to the transaction table
  m_pendingFibTransactions[currentTransactionId] = updates.size();

  for (Rib::FibUpdateList::const_iterator it = updates.begin(); it != updates.end(); ++it)
    {
      shared_ptr<const FibUpdate> update(*it);
      NFD_LOG_DEBUG("Sending FIB update: " << *update);

      if (update->action == FibUpdate::ADD_NEXTHOP)
        {
          FaceEntry faceEntry;
          faceEntry.faceId = update->faceId;
          faceEntry.cost = update->cost;

          m_nfdController.start<ndn::nfd::FibAddNextHopCommand>(
            ControlParameters()
              .setName(update->name)
              .setFaceId(faceEntry.faceId)
              .setCost(faceEntry.cost),
            bind(&RibManager::onAddNextHopSuccess, this, request,
                                                         parameters,
                                                         currentTransactionId,
                                                         shouldWaitToRespond),
            bind(&RibManager::onAddNextHopError, this, _1, _2, request, currentTransactionId,
                                                                        shouldWaitToRespond));
        }
      else if (update->action == FibUpdate::REMOVE_NEXTHOP)
        {
          FaceEntry faceEntry;
          faceEntry.faceId = update->faceId;

          m_nfdController.start<ndn::nfd::FibRemoveNextHopCommand>(
            ControlParameters()
              .setName(update->name)
              .setFaceId(faceEntry.faceId),
            bind(&RibManager::onRemoveNextHopSuccess, this, request,
                                                            parameters,
                                                            currentTransactionId,
                                                            shouldWaitToRespond),
            bind(&RibManager::onRemoveNextHopError, this, _1, _2, request, currentTransactionId,
                                                                           shouldWaitToRespond));
        }
    }

  m_managedRib.clearFibUpdates();
}

void
RibManager::sendUpdatesToFibAfterFaceDestroyEvent()
{
  ControlParameters parameters;
  parameters.setOrigin(ndn::nfd::ROUTE_ORIGIN_STATIC);

  sendUpdatesToFib(shared_ptr<const Interest>(), parameters);
}

void
RibManager::listEntries(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (commandNComps < LIST_COMMAND_NCOMPS ||
      !LIST_COMMAND_PREFIX.isPrefixOf(command))
    {
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
  if (finalBlockId.empty() || finalBlockId.toSegment() > currentSegment)
    {
      m_face.expressInterest(data.getName().getPrefix(-1).appendSegment(currentSegment+1),
                             bind(&RibManager::fetchSegments, this, _2, buffer),
                             bind(&RibManager::onFetchFaceStatusTimeout, this));
    }
  else
    {
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

  while (offset < buf->size())
    {
      if (!Block::fromBuffer(buf, offset, block))
        {
          std::cerr << "ERROR: cannot decode FaceStatus TLV" << std::endl;
          break;
        }

      offset += block.size();

      ndn::nfd::FaceStatus status(block);
      activeFaces.insert(status.getFaceId());
    }

  // Look for face IDs that were registered but not active to find missed
  // face destroyed events
  for (FaceIdSet::iterator it = m_registeredFaces.begin(); it != m_registeredFaces.end(); ++it)
    {
      if (activeFaces.find(*it) == activeFaces.end())
        {
          NFD_LOG_DEBUG("Removing invalid face ID: " << *it);
          scheduler::schedule(time::seconds(0),
                              bind(&RibManager::processErasureAfterNotification, this, *it));
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
