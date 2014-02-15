/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_NDNLP_SLICER_HPP
#define NFD_FACE_NDNLP_SLICER_HPP

#include "ndnlp-tlv.hpp"
#include "ndnlp-sequence-generator.hpp"

namespace nfd {
namespace ndnlp {

typedef shared_ptr<std::vector<Block> > PacketArray;

/** \brief provides fragmentation feature at sender
 */
class Slicer : noncopyable
{
public:
  explicit
  Slicer(size_t mtu);
  
  virtual
  ~Slicer();
  
  PacketArray
  slice(const Block& block);

private:
  /// estimate the size of NDNLP header and maximum payload size per packet
  void
  estimateOverhead();
  
private:
  SequenceGenerator m_seqgen;
  
  /// maximum packet size
  size_t m_mtu;
  
  /// maximum payload size
  size_t m_maxPayload;
};

} // namespace ndnlp
} // namespace nfd

#endif // NFD_FACE_NDNLP_SLICER_HPP
