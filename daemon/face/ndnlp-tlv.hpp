/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_NDNLP_TLV_HPP
#define NFD_FACE_NDNLP_TLV_HPP

namespace nfd {
namespace tlv {

enum
{
  NdnlpData      = 80,
  NdnlpSequence  = 81,
  NdnlpFragIndex = 82,
  NdnlpFragCount = 83,
  NdnlpPayload   = 84
};

} // namespace tlv
} // namespace nfd

#endif // NFD_FACE_NDNLP_TLV_HPP
