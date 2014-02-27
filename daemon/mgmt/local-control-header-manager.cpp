/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "local-control-header-manager.hpp"
#include "face/local-face.hpp"
#include "mgmt/internal-face.hpp"

namespace nfd {

NFD_LOG_INIT("LocalControlHeaderManager");

const Name LocalControlHeaderManager::COMMAND_PREFIX = "/localhost/nfd/control-header";

const size_t LocalControlHeaderManager::COMMAND_UNSIGNED_NCOMPS =
  LocalControlHeaderManager::COMMAND_PREFIX.size() +
  1 + // control-module
  1; // verb

const size_t LocalControlHeaderManager::COMMAND_SIGNED_NCOMPS =
  LocalControlHeaderManager::COMMAND_UNSIGNED_NCOMPS +
  4; // (timestamp, nonce, signed info tlv, signature tlv)


LocalControlHeaderManager::LocalControlHeaderManager(function<shared_ptr<Face>(FaceId)> getFace,
                                                     shared_ptr<InternalFace> face)
  : ManagerBase(face, CONTROL_HEADER_PRIVILEGE),
    m_getFace(getFace)
{
  face->setInterestFilter("/localhost/nfd/control-header",
                          bind(&LocalControlHeaderManager::onLocalControlHeaderRequest, this, _2));
}



void
LocalControlHeaderManager::onLocalControlHeaderRequest(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
      commandNComps < COMMAND_SIGNED_NCOMPS)
    {
      NFD_LOG_INFO("command result: unsigned verb: " << command);
      sendResponse(command, 401, "Signature required");

      return;
    }
  else if (commandNComps < COMMAND_SIGNED_NCOMPS ||
      !COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_INFO("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  validate(request,
             bind(&LocalControlHeaderManager::onCommandValidated,
                    this, _1),
             bind(&ManagerBase::onCommandValidationFailed,
                    this, _1, _2));


}

void
LocalControlHeaderManager::onCommandValidated(const shared_ptr<const Interest>& command)
{
  static const Name::Component MODULE_IN_FACEID("in-faceid");
  static const Name::Component MODULE_NEXTHOP_FACEID("nexthop-faceid");
  static const Name::Component VERB_ENABLE("enable");
  static const Name::Component VERB_DISABLE("disable");

  shared_ptr<LocalFace> face =
    dynamic_pointer_cast<LocalFace>(m_getFace(command->getIncomingFaceId()));

  if (!static_cast<bool>(face))
    {
      NFD_LOG_INFO("command result: command to enable control header on non-local face");
      sendResponse(command->getName(), 400, "Command not supported on the requested face");
      return;
    }

  const Name& commandName = command->getName();
  const Name::Component& module = commandName[COMMAND_PREFIX.size()];
  const Name::Component& verb = commandName[COMMAND_PREFIX.size() + 1];

  if (module == MODULE_IN_FACEID)
    {
      if (verb == VERB_ENABLE)
        {
          face->setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID, true);
          sendResponse(commandName, 200, "Success");
        }
      else if (verb == VERB_DISABLE)
        {
          face->setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID, false);
          sendResponse(commandName, 200, "Success");
        }
      else
        {
          NFD_LOG_INFO("command result: unsupported verb: " << verb);
          sendResponse(commandName, 501, "Unsupported");
        }
    }
  else if (module == MODULE_NEXTHOP_FACEID)
    {
      if (verb == VERB_ENABLE)
        {
          face->setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID, true);
          sendResponse(commandName, 200, "Success");
        }
      else if (verb == VERB_DISABLE)
        {
          face->setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID, false);
          sendResponse(commandName, 200, "Success");
        }
      else
        {
          NFD_LOG_INFO("command result: unsupported verb: " << verb);
          sendResponse(commandName, 501, "Unsupported");
        }
    }
  else
    {
      NFD_LOG_INFO("command result: unsupported module: " << module);
      sendResponse(commandName, 501, "Unsupported");
    }
}

} // namespace nfd

