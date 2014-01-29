/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_PIT_IN_RECORD_HPP
#define NFD_TABLE_PIT_IN_RECORD_HPP

#include "pit-face-record.hpp"

namespace nfd {
namespace pit {

/** \class InRecord
 *  \brief contains information about an Interest from an incoming face
 */
class InRecord : public FaceRecord
{
public:
  explicit
  InRecord(shared_ptr<Face> face);
  
  InRecord(const InRecord& other);
};

} // namespace pit
} // namespace nfd

#endif // NFD_TABLE_PIT_IN_RECORD_HPP
