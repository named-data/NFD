/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_PIT_OUT_RECORD_HPP
#define NFD_TABLE_PIT_OUT_RECORD_HPP

#include "pit-face-record.hpp"

namespace nfd {
namespace pit {

/** \class OutRecord
 *  \brief contains information about an Interest toward an outgoing face
 */
class OutRecord : public FaceRecord
{
public:
  explicit
  OutRecord(shared_ptr<Face> face);
  
  OutRecord(const OutRecord& other);
};

} // namespace pit
} // namespace nfd

#endif // NFD_TABLE_PIT_IN_RECORD_HPP
