/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology,
 *                     The University of Memphis
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
 **/

#include "rib-manager.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace rib {

NFD_LOG_INIT("RibManager");

const Name RibManager::COMMAND_PREFIX = "/localhost/nrd";
const Name RibManager::REMOTE_COMMAND_PREFIX = "/localhop/nrd";

const size_t RibManager::COMMAND_UNSIGNED_NCOMPS =
  RibManager::COMMAND_PREFIX.size() +
  1 + // verb
  1;  // verb options

const size_t RibManager::COMMAND_SIGNED_NCOMPS =
  RibManager::COMMAND_UNSIGNED_NCOMPS +
  4; // (timestamp, nonce, signed info tlv, signature tlv)

const RibManager::VerbAndProcessor RibManager::COMMAND_VERBS[] =
  {
    VerbAndProcessor(
                     Name::Component("register"),
                     &RibManager::insertEntry
                     ),

    VerbAndProcessor(
                     Name::Component("unregister"),
                     &RibManager::deleteEntry
                     ),
  };

RibManager::RibManager()
  : m_face(new ndn::Face())
  , m_nfdController(new ndn::nfd::Controller(*m_face))
  , m_validator(m_face)
  , m_faceMonitor(*m_face)
  , m_verbDispatch(COMMAND_VERBS,
                   COMMAND_VERBS + (sizeof(COMMAND_VERBS) / sizeof(VerbAndProcessor)))
{
}

void
RibManager::registerWithNfd()
{
  //check whether the components of localhop and localhost prefixes are same
  BOOST_ASSERT(COMMAND_PREFIX.size() == REMOTE_COMMAND_PREFIX.size());

  NFD_LOG_INFO("Setting interest filter on: " << COMMAND_PREFIX.toUri());
  m_face->setController(m_nfdController);
  m_face->setInterestFilter(COMMAND_PREFIX.toUri(),
                            bind(&RibManager::onRibRequest, this, _2),
                            bind(&RibManager::setInterestFilterFailed, this, _1, _2));

  NFD_LOG_INFO("Setting interest filter on: " << REMOTE_COMMAND_PREFIX.toUri());
  m_face->setInterestFilter(REMOTE_COMMAND_PREFIX.toUri(),
                            bind(&RibManager::onRibRequest, this, _2),
                            bind(&RibManager::setInterestFilterFailed, this, _1, _2));

  NFD_LOG_INFO("Start monitoring face create/destroy events");
  m_faceMonitor.addSubscriber(boost::bind(&RibManager::onNotification, this, _1));
  m_faceMonitor.startNotifications();
}

void
RibManager::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("rib_security",
                               bind(&RibManager::onConfig, this, _1, _2, _3));
}

void
RibManager::onConfig(const ConfigSection& configSection,
                      bool isDryRun,
                      const std::string& filename)
{
  /// \todo remove check after validator-conf replaces settings on each load
  if (!isDryRun)
    m_validator.load(configSection, filename);
}

void
RibManager::setInterestFilterFailed(const Name& name, const std::string& msg)
{
  NFD_LOG_ERROR("Error in setting interest filter (" << name << "): " << msg);
  m_face->shutdown();
}

void
RibManager::sendResponse(const Name& name,
                         const ControlResponse& response)
{
  const Block& encodedControl = response.wireEncode();

  Data responseData(name);
  responseData.setContent(encodedControl);

  m_keyChain.sign(responseData);
  m_face->put(responseData);
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
RibManager::onRibRequest(const Interest& request)
{
  m_validator.validate(request,
                       bind(&RibManager::onRibRequestValidated, this, _1),
                       bind(&RibManager::onRibRequestValidationFailed, this, _1, _2));
}

void
RibManager::onRibRequestValidated(const shared_ptr<const Interest>& request)
{
  const Name& command = request->getName();

  //REMOTE_COMMAND_PREFIX number of componenets are same as
  // NRD_COMMAND_PREFIX's so no extra checks are required.
  const Name::Component& verb = command.get(COMMAND_PREFIX.size());
  VerbDispatchTable::const_iterator verbProcessor = m_verbDispatch.find(verb);

  if (verbProcessor != m_verbDispatch.end())
    {
      NFD_LOG_TRACE("Processing '" << verb << "' verb");

      PrefixRegOptions options;
      if (!extractOptions(*request, options))
        {
          NFD_LOG_DEBUG("Error while extracting options, returning malformed command");
          sendResponse(command, 400, "Malformed command");
          return;
        }

      NFD_LOG_DEBUG("Received options (name, faceid, cost): " << options.getName()
                    << ", " << options.getFaceId() << ", "  << options.getCost());

      ControlResponse response;
      (verbProcessor->second)(this, *request, options);
    }
  else
    {
      NFD_LOG_DEBUG("Unsupported command: " << verb);
      sendResponse(request->getName(), 501, "Unsupported command");
    }
}

void
RibManager::onRibRequestValidationFailed(const shared_ptr<const Interest>& request,
                                         const std::string& failureInfo)
{
  NFD_LOG_DEBUG("RibRequestValidationFailed: " << failureInfo);
  sendResponse(request->getName(), 403, failureInfo);
}

bool
RibManager::extractOptions(const Interest& request,
                           PrefixRegOptions& extractedOptions)
{
  // const Name& command = request.getName();
  //REMOTE_COMMAND_PREFIX is same in size of NRD_COMMAND_PREFIX
  //so no extra processing is required.
  const size_t optionCompIndex = COMMAND_PREFIX.size() + 1;

  try
    {
      Block rawOptions = request.getName()[optionCompIndex].blockFromValue();
      extractedOptions.wireDecode(rawOptions);
    }
  catch (const ndn::Tlv::Error& e)
    {
      return false;
    }

  if (extractedOptions.getFaceId() == 0)
    {
      NFD_LOG_TRACE("IncomingFaceId: " << request.getIncomingFaceId());
      extractedOptions.setFaceId(request.getIncomingFaceId());
    }
  return true;
}

void
RibManager::onCommandError(uint32_t code, const std::string& error,
                           const Interest& request,
                           const PrefixRegOptions& options)
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

  sendResponse(request.getName(), response);
  m_managedRib.erase(options);
}

void
RibManager::onUnRegSuccess(const Interest& request, const PrefixRegOptions& options)
{
  ControlResponse response;

  response.setCode(200);
  response.setText("Success");
  response.setBody(options.wireEncode());

  NFD_LOG_DEBUG("onUnRegSuccess: Name unregistered (" << options.getName()
                << ", " << options.getFaceId() << ")");

  sendResponse(request.getName(), response);
  m_managedRib.erase(options);
}

void
RibManager::onRegSuccess(const Interest& request, const PrefixRegOptions& options)
{
  ControlResponse response;

  response.setCode(200);
  response.setText("Success");
  response.setBody(options.wireEncode());

  NFD_LOG_DEBUG("onRegSuccess: Name registered (" << options.getName() << ", "
                << options.getFaceId() << ")");
  sendResponse(request.getName(), response);
}

void
RibManager::insertEntry(const Interest& request, const PrefixRegOptions& options)
{
  // For right now, just pass the options to fib as it is,
  // without processing flags. Later options will be first added to
  // Rib tree, then nrd will generate fib updates based on flags and then
  // will add next hops one by one..
  m_managedRib.insert(options);
  m_nfdController->start<ndn::nfd::FibAddNextHopCommand>(
    ControlParameters()
      .setName(options.getName())
      .setFaceId(options.getFaceId())
      .setCost(options.getCost()),
    bind(&RibManager::onRegSuccess, this, request, options),
    bind(&RibManager::onCommandError, this, _1, _2, request, options));
}

void
RibManager::deleteEntry(const Interest& request, const PrefixRegOptions& options)
{
  m_nfdController->start<ndn::nfd::FibRemoveNextHopCommand>(
    ControlParameters()
      .setName(options.getName())
      .setFaceId(options.getFaceId()),
    bind(&RibManager::onUnRegSuccess, this, request, options),
    bind(&RibManager::onCommandError, this, _1, _2, request, options));
}

boost::asio::io_service&
RibManager::getIoService()
{
  /// \todo Switch face to use global io service (needs library update)
  return *m_face->ioService();
}

void
RibManager::onControlHeaderSuccess()
{
  NFD_LOG_DEBUG("Local control header enabled");
}

void
RibManager::onControlHeaderError(uint32_t code, const std::string& reason)
{
  NFD_LOG_ERROR("Error: couldn't enable local control header "
                << "(code: " << code << ", info: " << reason << ")");
  m_face->shutdown();
}

void
RibManager::enableLocalControlHeader()
{
  m_nfdController->start<ndn::nfd::FaceEnableLocalControlCommand>(
    ControlParameters()
      .setLocalControlFeature(ndn::nfd::LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID),
    bind(&RibManager::onControlHeaderSuccess, this),
    bind(&RibManager::onControlHeaderError, this, _1, _2));
}

void
RibManager::onNotification(const FaceEventNotification& notification)
{
  /// \todo A notification can be missed, in this case check Facelist
  NFD_LOG_TRACE("onNotification: " << notification);
  if (notification.getKind() == ndn::nfd::FACE_EVENT_DESTROYED) { //face destroyed
    m_managedRib.erase(notification.getFaceId());
  }
}

} // namespace rib
} // namespace nfd
