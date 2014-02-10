/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "manager-base.hpp"
#include "mgmt/app-face.hpp"

namespace nfd {

NFD_LOG_INIT("ManagerBase");

ManagerBase::ManagerBase(shared_ptr<AppFace> face)
  : m_face(face)
{

}

ManagerBase::~ManagerBase()
{

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
                          const ControlResponse& response)
{
  NFD_LOG_DEBUG("responding"
                << " name: " << name
                << " code: " << response.getCode()
                << " text: " << response.getText());

  const Block& encodedControl = response.wireEncode();

  Data responseData(name);
  responseData.setContent(encodedControl);

  m_face->sign(responseData);
  m_face->put(responseData);
}


} // namespace nfd
