/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "manager-base.hpp"
#include "mgmt/app-face.hpp"

#include <ndn-cpp-dev/management/control-response.hpp>

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
  ndn::ControlResponse control(code, text);
  const Block& encodedControl = control.wireEncode();

  NFD_LOG_DEBUG("sending control response"
                << " Name: " << name
                << " code: " << code
                << " text: " << text);

  Data response(name);
  response.setContent(encodedControl);

  m_keyChain.sign(response);
  m_face->put(response);
}


} // namespace nfd
