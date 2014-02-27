/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_LOCAL_CONTROL_HEADER_MANAGER_HPP
#define NFD_MGMT_LOCAL_CONTROL_HEADER_MANAGER_HPP

#include "common.hpp"
#include "face/face.hpp"
#include "mgmt/app-face.hpp"
#include "mgmt/manager-base.hpp"

namespace nfd {

const std::string CONTROL_HEADER_PRIVILEGE = "control-header"; // config file privilege name

class LocalControlHeaderManager : public ManagerBase
{
public:
  LocalControlHeaderManager(function<shared_ptr<Face>(FaceId)> getFace,
                            shared_ptr<InternalFace> face);

  void
  onLocalControlHeaderRequest(const Interest& request);

  void
  onCommandValidated(const shared_ptr<const Interest>& command);

private:
  function<shared_ptr<Face>(FaceId)> m_getFace;

  static const Name COMMAND_PREFIX; // /localhost/nfd/control-header

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nfd/control-header + control-module + verb) = 5
  static const size_t COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // UNSIGNED_NCOMPS + 4 command Interest components = 9
  static const size_t COMMAND_SIGNED_NCOMPS;
};

} // namespace nfd

#endif // NFD_MGMT_LOCAL_CONTROL_HEADER_MANAGER_HPP



