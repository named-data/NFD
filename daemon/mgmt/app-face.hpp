/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_APP_FACE_HPP
#define NFD_MGMT_APP_FACE_HPP

#include "common.hpp"

namespace nfd {

typedef ndn::func_lib::function<void(const Name&, const Interest&)> OnInterest;

class AppFace
{
public:
  virtual void
  setInterestFilter(const Name& filter,
                    OnInterest onInterest) = 0;

  virtual void
  put(const Data& data) = 0;

  virtual
  ~AppFace() { }
};

} // namespace nfd

#endif //NFD_MGMT_APP_FACE_HPP
