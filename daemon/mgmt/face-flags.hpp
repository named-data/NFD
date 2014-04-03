/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_FACE_FLAGS_HPP
#define NFD_MGMT_FACE_FLAGS_HPP

#include "face/face.hpp"
#include <ndn-cpp-dev/management/nfd-face-flags.hpp>

namespace nfd {

inline uint64_t
getFaceFlags(const Face& face)
{
  uint64_t flags = 0;
  if (face.isLocal()) {
    flags |= ndn::nfd::FACE_IS_LOCAL;
  }
  if (face.isOnDemand()) {
    flags |= ndn::nfd::FACE_IS_ON_DEMAND;
  }
  return flags;
}

} // namespace nfd

#endif // NFD_MGMT_FACE_FLAGS_HPP
