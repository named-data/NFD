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

#include "manager-base.hpp"
#include "core/logger.hpp"

namespace nfd {

NFD_LOG_INIT("ManagerBase");

ManagerBase::ManagerBase(shared_ptr<InternalFace> face, const std::string& privilege,
                         ndn::KeyChain& keyChain)
  : m_face(face)
  , m_keyChain(keyChain)
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
  catch (const tlv::Error&)
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

  m_keyChain.sign(*responseData);
  m_face->put(*responseData);
}

void
ManagerBase::sendNack(const Name& name)
{
  NFD_LOG_DEBUG("responding NACK to " << name);

  ndn::MetaInfo meta;
  meta.setType(tlv::ContentType_Nack);

  shared_ptr<Data> responseData(make_shared<Data>(name));
  responseData->setMetaInfo(meta);

  m_keyChain.sign(*responseData);
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
  NFD_LOG_DEBUG("command result: unauthorized command: " << *command << " (" << error << ")");
  sendResponse(command->getName(), 403, "Unauthorized command");
}


} // namespace nfd
