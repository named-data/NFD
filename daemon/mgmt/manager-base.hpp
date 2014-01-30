/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_MANAGER_BASE_HPP
#define NFD_MGMT_MANAGER_BASE_HPP

#include "common.hpp"

namespace nfd {

class AppFace;

class ManagerBase
{
public:
  ManagerBase(shared_ptr<AppFace> face);

  virtual
  ~ManagerBase();

protected:

  virtual void
  sendResponse(const Name& name,
                 uint32_t code,
                 const std::string& text);

protected:
  shared_ptr<AppFace> m_face;
};


} // namespace ndf

#endif // NFD_MGMT_MANAGER_BASE_HPP

