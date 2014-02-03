/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_APP_FACE_HPP
#define NFD_MGMT_APP_FACE_HPP

#include "common.hpp"

#include <ndn-cpp-dev/security/key-chain.hpp>

namespace nfd {

typedef function<void(const Name&, const Interest&)> OnInterest;

class AppFace
{
public:
  virtual void
  setInterestFilter(const Name& filter,
                    OnInterest onInterest) = 0;

  virtual void
  sign(Data& data);

  virtual void
  put(const Data& data) = 0;

  virtual
  ~AppFace() { }

protected:
  ndn::KeyChain m_keyChain;
};

} // namespace nfd

#endif //NFD_MGMT_APP_FACE_HPP
