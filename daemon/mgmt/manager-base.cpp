/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "manager-base.hpp"
#include "core/logger.hpp"

namespace nfd {

NFD_LOG_INIT("ManagerBase");

ManagerBase::ManagerBase(shared_ptr<InternalFace> face, const std::string& privilege)
  : m_face(face)
{
  face->getValidator().addSupportedPrivilege(privilege);
}

ManagerBase::~ManagerBase()
{

}

bool
ManagerBase::extractParameters(const Name::Component& parameterComponent,
                               ControlParameters& extractedParameters)
{
  try
    {
      Block rawParameters = parameterComponent.blockFromValue();
      extractedParameters.wireDecode(rawParameters);
    }
  catch (const ndn::Tlv::Error& e)
    {
      return false;
    }

  NFD_LOG_DEBUG("Parameters parsed OK");
  return true;
}

void
ManagerBase::sendResponse(const Name& name,
                          uint32_t code,
                          const std::string& text)
{
  ControlResponse response(code, text);
  sendResponse(name, response);
}

void
ManagerBase::sendResponse(const Name& name,
                          uint32_t code,
                          const std::string& text,
                          const Block& body)
{
  ControlResponse response(code, text);
  response.setBody(body);
  sendResponse(name, response);
}

void
ManagerBase::sendResponse(const Name& name,
                          const ControlResponse& response)
{
  NFD_LOG_DEBUG("responding"
                << " name: " << name
                << " code: " << response.getCode()
                << " text: " << response.getText());

  const Block& encodedControl = response.wireEncode();

  shared_ptr<Data> responseData(make_shared<Data>(name));
  responseData->setContent(encodedControl);

  m_face->sign(*responseData);
  m_face->put(*responseData);
}

bool
ManagerBase::validateParameters(const ControlCommand& command,
                                ControlParameters& parameters)
{
  try
    {
      command.validateRequest(parameters);
    }
  catch (const ControlCommand::ArgumentError& error)
    {
      return false;
    }

  command.applyDefaultsToRequest(parameters);

  return true;
}

void
ManagerBase::onCommandValidationFailed(const shared_ptr<const Interest>& command,
                                       const std::string& error)
{
  NFD_LOG_INFO("command result: unauthorized command: " << *command);
  sendResponse(command->getName(), 403, "Unauthorized command");
}


} // namespace nfd
