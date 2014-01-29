/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "pit-out-record.hpp"

namespace nfd {
namespace pit {

OutRecord::OutRecord(shared_ptr<Face> face)
  : FaceRecord(face)
{
}

OutRecord::OutRecord(const OutRecord& other)
  : FaceRecord(other)
{
}

} // namespace pit
} // namespace nfd
