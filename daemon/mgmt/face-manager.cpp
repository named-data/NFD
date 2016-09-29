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

#include "face-manager.hpp"

#include "core/network-interface.hpp"
#include "face/generic-link-service.hpp"
#include "face/tcp-factory.hpp"
#include "face/udp-factory.hpp"
#include "fw/face-table.hpp"

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/channel-status.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/mgmt/nfd/face-event-notification.hpp>

#ifdef HAVE_UNIX_SOCKETS
#include "face/unix-stream-factory.hpp"
#endif // HAVE_UNIX_SOCKETS

#ifdef HAVE_LIBPCAP
#include "face/ethernet-factory.hpp"
#include "face/ethernet-transport.hpp"
#endif // HAVE_LIBPCAP

#ifdef HAVE_WEBSOCKET
#include "face/websocket-factory.hpp"
#endif // HAVE_WEBSOCKET

namespace nfd {

NFD_LOG_INIT("FaceManager");

FaceManager::FaceManager(FaceTable& faceTable, Dispatcher& dispatcher, CommandAuthenticator& authenticator)
  : NfdManagerBase(dispatcher, authenticator, "faces")
  , m_faceTable(faceTable)
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
  m_faceAddConn = m_faceTable.afterAdd.connect(bind(&FaceManager::notifyAddFace, this, _1));
  m_faceRemoveConn = m_faceTable.beforeRemove.connect(bind(&FaceManager::notifyRemoveFace, this, _1));
}

void
FaceManager::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("face_system", bind(&FaceManager::processConfig, this, _1, _2, _3));
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

  auto factory = m_factories.find(uri.getScheme());
  if (factory == m_factories.end()) {
    NFD_LOG_TRACE("received create request for unsupported protocol");
    done(ControlResponse(406, "Unsupported protocol"));
    return;
  }

  try {
    factory->second->createFace(uri,
                                parameters.getFacePersistency(),
                                parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) ?
                                  parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) : false,
                                bind(&FaceManager::afterCreateFaceSuccess,
                                     this, parameters, _1, done),
                                bind(&FaceManager::afterCreateFaceFailure,
                                     this, _1, _2, done));
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
  std::set<const ProtocolFactory*> seenFactories;

  for (const auto& kv : m_factories) {
    const ProtocolFactory* factory = kv.second.get();
    bool inserted;
    std::tie(std::ignore, inserted) = seenFactories.insert(factory);

    if (inserted) {
      for (const auto& channel : factory->getChannels()) {
        ndn::nfd::ChannelStatus entry;
        entry.setLocalUri(channel->getUri().toString());
        context.append(entry.wireEncode());
      }
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

  // Set Flag bits
  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  if (linkService != nullptr) {
    auto linkServiceOptions = linkService->getOptions();
    status.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, linkServiceOptions.allowLocalFields);
  }

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
}

void
FaceManager::notifyAddFace(const Face& face)
{
  ndn::nfd::FaceEventNotification notification;
  notification.setKind(ndn::nfd::FACE_EVENT_CREATED);
  collectFaceProperties(face, notification);

  m_postNotification(notification.wireEncode());
}

void
FaceManager::notifyRemoveFace(const Face& face)
{
  ndn::nfd::FaceEventNotification notification;
  notification.setKind(ndn::nfd::FACE_EVENT_DESTROYED);
  collectFaceProperties(face, notification);

  m_postNotification(notification.wireEncode());
}

void
FaceManager::processConfig(const ConfigSection& configSection,
                           bool isDryRun,
                           const std::string& filename)
{
  bool hasSeenUnix = false;
  bool hasSeenTcp = false;
  bool hasSeenUdp = false;
  bool hasSeenEther = false;
  bool hasSeenWebSocket = false;
  auto nicList = listNetworkInterfaces();

  for (const auto& item : configSection) {
    if (item.first == "unix") {
      if (hasSeenUnix) {
        BOOST_THROW_EXCEPTION(Error("Duplicate \"unix\" section"));
      }
      hasSeenUnix = true;

      processSectionUnix(item.second, isDryRun);
    }
    else if (item.first == "tcp") {
      if (hasSeenTcp) {
        BOOST_THROW_EXCEPTION(Error("Duplicate \"tcp\" section"));
      }
      hasSeenTcp = true;

      processSectionTcp(item.second, isDryRun);
    }
    else if (item.first == "udp") {
      if (hasSeenUdp) {
        BOOST_THROW_EXCEPTION(Error("Duplicate \"udp\" section"));
      }
      hasSeenUdp = true;

      processSectionUdp(item.second, isDryRun, nicList);
    }
    else if (item.first == "ether") {
      if (hasSeenEther) {
        BOOST_THROW_EXCEPTION(Error("Duplicate \"ether\" section"));
      }
      hasSeenEther = true;

      processSectionEther(item.second, isDryRun, nicList);
    }
    else if (item.first == "websocket") {
      if (hasSeenWebSocket) {
        BOOST_THROW_EXCEPTION(Error("Duplicate \"websocket\" section"));
      }
      hasSeenWebSocket = true;

      processSectionWebSocket(item.second, isDryRun);
    }
    else {
      BOOST_THROW_EXCEPTION(Error("Unrecognized option \"" + item.first + "\""));
    }
  }
}

void
FaceManager::processSectionUnix(const ConfigSection& configSection, bool isDryRun)
{
  // ; the unix section contains settings of Unix stream faces and channels
  // unix
  // {
  //   path /var/run/nfd.sock ; Unix stream listener path
  // }

#if defined(HAVE_UNIX_SOCKETS)
  std::string path = "/var/run/nfd.sock";

  for (const auto& i : configSection) {
    if (i.first == "path") {
      path = i.second.get_value<std::string>();
    }
    else {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option \"" +
                                              i.first + "\" in \"unix\" section"));
    }
  }

  if (!isDryRun) {
    if (m_factories.count("unix") > 0) {
      return;
    }

    auto factory = make_shared<UnixStreamFactory>();
    m_factories.insert(std::make_pair("unix", factory));

    auto channel = factory->createChannel(path);
    channel->listen(bind(&FaceTable::add, &m_faceTable, _1), nullptr);
  }
#else
  BOOST_THROW_EXCEPTION(ConfigFile::Error("NFD was compiled without Unix sockets support, "
                                          "cannot process \"unix\" section"));
#endif // HAVE_UNIX_SOCKETS
}

void
FaceManager::processSectionTcp(const ConfigSection& configSection, bool isDryRun)
{
  // ; the tcp section contains settings of TCP faces and channels
  // tcp
  // {
  //   listen yes ; set to 'no' to disable TCP listener, default 'yes'
  //   port 6363 ; TCP listener port number
  // }

  uint16_t port = 6363;
  bool needToListen = true;
  bool enableV4 = true;
  bool enableV6 = true;

  for (const auto& i : configSection) {
    if (i.first == "port") {
      port = ConfigFile::parseNumber<uint16_t>(i, "tcp");
      NFD_LOG_TRACE("TCP port set to " << port);
    }
    else if (i.first == "listen") {
      needToListen = ConfigFile::parseYesNo(i, "tcp");
    }
    else if (i.first == "enable_v4") {
      enableV4 = ConfigFile::parseYesNo(i, "tcp");
    }
    else if (i.first == "enable_v6") {
      enableV6 = ConfigFile::parseYesNo(i, "tcp");
    }
    else {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option \"" +
                                              i.first + "\" in \"tcp\" section"));
    }
  }

  if (!enableV4 && !enableV6) {
    BOOST_THROW_EXCEPTION(ConfigFile::Error("IPv4 and IPv6 TCP channels have been disabled."
                                            " Remove \"tcp\" section to disable TCP channels or"
                                            " re-enable at least one channel type."));
  }

  if (!isDryRun) {
    if (m_factories.count("tcp") > 0) {
      return;
    }

    auto factory = make_shared<TcpFactory>();
    m_factories.insert(std::make_pair("tcp", factory));

    if (enableV4) {
      tcp::Endpoint endpoint(boost::asio::ip::tcp::v4(), port);
      shared_ptr<TcpChannel> v4Channel = factory->createChannel(endpoint);
      if (needToListen) {
        v4Channel->listen(bind(&FaceTable::add, &m_faceTable, _1), nullptr);
      }

      m_factories.insert(std::make_pair("tcp4", factory));
    }

    if (enableV6) {
      tcp::Endpoint endpoint(boost::asio::ip::tcp::v6(), port);
      shared_ptr<TcpChannel> v6Channel = factory->createChannel(endpoint);
      if (needToListen) {
        v6Channel->listen(bind(&FaceTable::add, &m_faceTable, _1), nullptr);
      }

      m_factories.insert(std::make_pair("tcp6", factory));
    }
  }
}

void
FaceManager::processSectionUdp(const ConfigSection& configSection, bool isDryRun,
                               const std::vector<NetworkInterfaceInfo>& nicList)
{
  // ; the udp section contains settings of UDP faces and channels
  // udp
  // {
  //   port 6363 ; UDP unicast port number
  //   idle_timeout 600 ; idle time (seconds) before closing a UDP unicast face
  //   keep_alive_interval 25 ; interval (seconds) between keep-alive refreshes

  //   ; NFD creates one UDP multicast face per NIC
  //   mcast yes ; set to 'no' to disable UDP multicast, default 'yes'
  //   mcast_port 56363 ; UDP multicast port number
  //   mcast_group 224.0.23.170 ; UDP multicast group (IPv4 only)
  // }

  uint16_t port = 6363;
  bool enableV4 = true;
  bool enableV6 = true;
  size_t timeout = 600;
  size_t keepAliveInterval = 25;
  bool useMcast = true;
  auto mcastGroup = boost::asio::ip::address_v4::from_string("224.0.23.170");
  uint16_t mcastPort = 56363;

  for (const auto& i : configSection) {
    if (i.first == "port") {
      port = ConfigFile::parseNumber<uint16_t>(i, "udp");
      NFD_LOG_TRACE("UDP unicast port set to " << port);
    }
    else if (i.first == "enable_v4") {
      enableV4 = ConfigFile::parseYesNo(i, "udp");
    }
    else if (i.first == "enable_v6") {
      enableV6 = ConfigFile::parseYesNo(i, "udp");
    }
    else if (i.first == "idle_timeout") {
      try {
        timeout = i.second.get_value<size_t>();
      }
      catch (const boost::property_tree::ptree_bad_data&) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for option \"" +
                                                i.first + "\" in \"udp\" section"));
      }
    }
    else if (i.first == "keep_alive_interval") {
      try {
        keepAliveInterval = i.second.get_value<size_t>();
        /// \todo Make use of keepAliveInterval
        (void)(keepAliveInterval);
      }
      catch (const boost::property_tree::ptree_bad_data&) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for option \"" +
                                                i.first + "\" in \"udp\" section"));
      }
    }
    else if (i.first == "mcast") {
      useMcast = ConfigFile::parseYesNo(i, "udp");
    }
    else if (i.first == "mcast_port") {
      mcastPort = ConfigFile::parseNumber<uint16_t>(i, "udp");
      NFD_LOG_TRACE("UDP multicast port set to " << mcastPort);
    }
    else if (i.first == "mcast_group") {
      boost::system::error_code ec;
      mcastGroup = boost::asio::ip::address_v4::from_string(i.second.get_value<std::string>(), ec);
      if (ec) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for option \"" +
                                                i.first + "\" in \"udp\" section"));
      }
      NFD_LOG_TRACE("UDP multicast group set to " << mcastGroup);
    }
    else {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option \"" +
                                              i.first + "\" in \"udp\" section"));
    }
  }

  if (!enableV4 && !enableV6) {
    BOOST_THROW_EXCEPTION(ConfigFile::Error("IPv4 and IPv6 UDP channels have been disabled."
                                            " Remove \"udp\" section to disable UDP channels or"
                                            " re-enable at least one channel type."));
  }
  else if (useMcast && !enableV4) {
    BOOST_THROW_EXCEPTION(ConfigFile::Error("IPv4 multicast requested, but IPv4 channels"
                                            " have been disabled (conflicting configuration options set)"));
  }

  if (!isDryRun) {
    shared_ptr<UdpFactory> factory;
    bool isReload = false;
    if (m_factories.count("udp") > 0) {
      isReload = true;
      factory = static_pointer_cast<UdpFactory>(m_factories["udp"]);
    }
    else {
      factory = make_shared<UdpFactory>();
      m_factories.insert(std::make_pair("udp", factory));
    }

    if (!isReload && enableV4) {
      udp::Endpoint endpoint(boost::asio::ip::udp::v4(), port);
      shared_ptr<UdpChannel> v4Channel = factory->createChannel(endpoint, time::seconds(timeout));
      v4Channel->listen(bind(&FaceTable::add, &m_faceTable, _1), nullptr);

      m_factories.insert(std::make_pair("udp4", factory));
    }

    if (!isReload && enableV6) {
      udp::Endpoint endpoint(boost::asio::ip::udp::v6(), port);
      shared_ptr<UdpChannel> v6Channel = factory->createChannel(endpoint, time::seconds(timeout));
      v6Channel->listen(bind(&FaceTable::add, &m_faceTable, _1), nullptr);

      m_factories.insert(std::make_pair("udp6", factory));
    }

    std::set<shared_ptr<Face>> multicastFacesToRemove;
    for (const auto& i : factory->getMulticastFaces()) {
      multicastFacesToRemove.insert(i.second);
    }

    if (useMcast && enableV4) {
      std::vector<NetworkInterfaceInfo> ipv4MulticastInterfaces;
      for (const auto& nic : nicList) {
        if (nic.isUp() && nic.isMulticastCapable() && !nic.ipv4Addresses.empty()) {
          ipv4MulticastInterfaces.push_back(nic);
        }
      }

      bool isNicNameNecessary = false;
#if defined(__linux__)
      if (ipv4MulticastInterfaces.size() > 1) {
        // On Linux if we have more than one MulticastUdpFace
        // we need to specify the name of the interface
        isNicNameNecessary = true;
      }
#endif

      udp::Endpoint mcastEndpoint(mcastGroup, mcastPort);
      for (const auto& nic : ipv4MulticastInterfaces) {
        udp::Endpoint localEndpoint(nic.ipv4Addresses[0], mcastPort);
        auto newFace = factory->createMulticastFace(localEndpoint, mcastEndpoint,
                                                    isNicNameNecessary ? nic.name : "");
        m_faceTable.add(newFace);
        multicastFacesToRemove.erase(newFace);
      }
    }

    for (const auto& face : multicastFacesToRemove) {
      face->close();
    }
  }
}

void
FaceManager::processSectionEther(const ConfigSection& configSection, bool isDryRun,
                                 const std::vector<NetworkInterfaceInfo>& nicList)
{
  // ; the ether section contains settings of Ethernet faces and channels
  // ether
  // {
  //   ; NFD creates one Ethernet multicast face per NIC
  //   mcast yes ; set to 'no' to disable Ethernet multicast, default 'yes'
  //   mcast_group 01:00:5E:00:17:AA ; Ethernet multicast group
  // }

#if defined(HAVE_LIBPCAP)
  bool useMcast = true;
  ethernet::Address mcastGroup(ethernet::getDefaultMulticastAddress());

  for (const auto& i : configSection) {
    if (i.first == "mcast") {
      useMcast = ConfigFile::parseYesNo(i, "ether");
    }
    else if (i.first == "mcast_group") {
      mcastGroup = ethernet::Address::fromString(i.second.get_value<std::string>());
      if (mcastGroup.isNull()) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Invalid value for option \"" +
                                                i.first + "\" in \"ether\" section"));
      }
      NFD_LOG_TRACE("Ethernet multicast group set to " << mcastGroup);
    }
    else {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option \"" +
                                              i.first + "\" in \"ether\" section"));
    }
  }

  if (!isDryRun) {
    shared_ptr<EthernetFactory> factory;
    if (m_factories.count("ether") > 0) {
      factory = static_pointer_cast<EthernetFactory>(m_factories["ether"]);
    }
    else {
      factory = make_shared<EthernetFactory>();
      m_factories.insert(std::make_pair("ether", factory));
    }

    std::set<shared_ptr<Face>> multicastFacesToRemove;
    for (const auto& i : factory->getMulticastFaces()) {
      multicastFacesToRemove.insert(i.second);
    }

    if (useMcast) {
      for (const auto& nic : nicList) {
        if (nic.isUp() && nic.isMulticastCapable()) {
          try {
            auto newFace = factory->createMulticastFace(nic, mcastGroup);
            m_faceTable.add(newFace);
            multicastFacesToRemove.erase(newFace);
          }
          catch (const EthernetFactory::Error& factoryError) {
            NFD_LOG_ERROR(factoryError.what() << ", continuing");
          }
          catch (const face::EthernetTransport::Error& faceError) {
            NFD_LOG_ERROR(faceError.what() << ", continuing");
          }
        }
      }
    }

    for (const auto& face : multicastFacesToRemove) {
      face->close();
    }
  }
#else
  BOOST_THROW_EXCEPTION(ConfigFile::Error("NFD was compiled without libpcap, cannot process \"ether\" section"));
#endif // HAVE_LIBPCAP
}

void
FaceManager::processSectionWebSocket(const ConfigSection& configSection, bool isDryRun)
{
  // ; the websocket section contains settings of WebSocket faces and channels
  // websocket
  // {
  //   listen yes ; set to 'no' to disable WebSocket listener, default 'yes'
  //   port 9696 ; WebSocket listener port number
  //   enable_v4 yes ; set to 'no' to disable listening on IPv4 socket, default 'yes'
  //   enable_v6 yes ; set to 'no' to disable listening on IPv6 socket, default 'yes'
  // }

#if defined(HAVE_WEBSOCKET)
  uint16_t port = 9696;
  bool needToListen = true;
  bool enableV4 = true;
  bool enableV6 = true;

  for (const auto& i : configSection) {
    if (i.first == "port") {
      port = ConfigFile::parseNumber<uint16_t>(i, "websocket");
      NFD_LOG_TRACE("WebSocket port set to " << port);
    }
    else if (i.first == "listen") {
      needToListen = ConfigFile::parseYesNo(i, "websocket");
    }
    else if (i.first == "enable_v4") {
      enableV4 = ConfigFile::parseYesNo(i, "websocket");
    }
    else if (i.first == "enable_v6") {
      enableV6 = ConfigFile::parseYesNo(i, "websocket");
    }
    else {
      BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized option \"" +
                                              i.first + "\" in \"websocket\" section"));
    }
  }

  if (!enableV4 && !enableV6) {
    BOOST_THROW_EXCEPTION(ConfigFile::Error("IPv4 and IPv6 WebSocket channels have been disabled."
                                            " Remove \"websocket\" section to disable WebSocket channels or"
                                            " re-enable at least one channel type."));
  }

  if (!enableV4 && enableV6) {
    BOOST_THROW_EXCEPTION(ConfigFile::Error("NFD does not allow pure IPv6 WebSocket channel."));
  }

  if (!isDryRun) {
    if (m_factories.count("websocket") > 0) {
      return;
    }

    auto factory = make_shared<WebSocketFactory>();
    m_factories.insert(std::make_pair("websocket", factory));

    shared_ptr<WebSocketChannel> channel;

    if (enableV6 && enableV4) {
      websocket::Endpoint endpoint(boost::asio::ip::address_v6::any(), port);
      channel = factory->createChannel(endpoint);

      m_factories.insert(std::make_pair("websocket46", factory));
    }
    else if (enableV4) {
      websocket::Endpoint endpoint(boost::asio::ip::address_v4::any(), port);
      channel = factory->createChannel(endpoint);

      m_factories.insert(std::make_pair("websocket4", factory));
    }

    if (channel && needToListen) {
      channel->listen(bind(&FaceTable::add, &m_faceTable, _1));
    }
  }
#else
  BOOST_THROW_EXCEPTION(ConfigFile::Error("NFD was compiled without WebSocket, "
                                          "cannot process \"websocket\" section"));
#endif // HAVE_WEBSOCKET
}

} // namespace nfd
