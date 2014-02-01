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
  // Path 1 - runtime exception on receive's wireDecode.
  // Set Data response's content payload to the
  // encoded Block.
  {
    ndn::ControlResponse control(code, text);
    const Block& encodedControl = control.wireEncode();

    NFD_LOG_DEBUG("sending control response (Path 1)"
                  << " Name: " << name
                  << " code: " << code
                  << " text: " << text);

    NFD_LOG_DEBUG("sending raw control block size = " << encodedControl.size());

    Data response(name);
    response.setContent(encodedControl);

    m_face->put(response);
  }

  // Path 2 - works, but not conformant to protocol.
  // Encode ControlResponse and append Block as
  // the last component of the name.
  // {
  //   ndn::ControlResponse control(code, text);
  //   const Block& encodedControl = control.wireEncode();

  //   NFD_LOG_DEBUG("sending control response (Path 2)"
  //                 << " Name: " << name
  //                 << " code: " << code
  //                 << " text: " << text);

  //   NFD_LOG_DEBUG("sending raw control block size = " << encodedControl.size());

  //   Name responseName(name);
  //   responseName.append(encodedControl);

  //   Data response(responseName);
  //   m_face->put(response);
  // }

}


} // namespace nfd
