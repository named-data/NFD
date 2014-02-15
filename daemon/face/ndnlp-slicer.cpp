/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "ndnlp-slicer.hpp"

#include <ndn-cpp-dev/encoding/encoding-buffer.hpp>

namespace nfd {
namespace ndnlp {

Slicer::Slicer(size_t mtu)
  : m_mtu(mtu)
{
  this->estimateOverhead();
}

Slicer::~Slicer()
{
}

template<bool T>
static inline size_t
Slicer_encodeFragment(ndn::EncodingImpl<T>& blk,
                      uint64_t seq, uint16_t fragIndex, uint16_t fragCount,
                      const uint8_t* payload, size_t payloadSize)
{
  size_t totalLength = 0;
  
  // NdnlpPayload
  size_t payloadLength = blk.prependByteArray(payload, payloadSize);
  totalLength += payloadLength;
  totalLength += blk.prependVarNumber(payloadLength);
  totalLength += blk.prependVarNumber(tlv::NdnlpPayload);
  
  bool needFragIndexAndCount = fragCount > 1;
  if (needFragIndexAndCount) {
    // NdnlpFragCount
    uint16_t fragCountBE = htobe16(fragCount);
    size_t fragCountLength = blk.prependByteArray(
      reinterpret_cast<uint8_t*>(&fragCountBE), sizeof(fragCountBE));
    totalLength += fragCountLength;
    totalLength += blk.prependVarNumber(fragCountLength);
    totalLength += blk.prependVarNumber(tlv::NdnlpFragCount);
    
    // NdnlpFragIndex
    uint16_t fragIndexBE = htobe16(fragIndex);
    size_t fragIndexLength = blk.prependByteArray(
      reinterpret_cast<uint8_t*>(&fragIndexBE), sizeof(fragIndexBE));
    totalLength += fragIndexLength;
    totalLength += blk.prependVarNumber(fragIndexLength);
    totalLength += blk.prependVarNumber(tlv::NdnlpFragIndex);
  }
  
  // NdnlpSequence
  uint64_t sequenceBE = htobe64(seq);
  size_t sequenceLength = blk.prependByteArray(
    reinterpret_cast<uint8_t*>(&sequenceBE), sizeof(sequenceBE));
  totalLength += sequenceLength;
  totalLength += blk.prependVarNumber(sequenceLength);
  totalLength += blk.prependVarNumber(tlv::NdnlpSequence);
  
  totalLength += blk.prependVarNumber(totalLength);
  totalLength += blk.prependVarNumber(tlv::NdnlpData);
  
  return totalLength;
}

void
Slicer::estimateOverhead()
{
  ndn::EncodingEstimator estimator;
  size_t estimatedSize = Slicer_encodeFragment(estimator,
                         0, 0, 2, 0, m_mtu);
  
  size_t overhead = estimatedSize - m_mtu;
  m_maxPayload = m_mtu - overhead;
}

PacketArray
Slicer::slice(const Block& block)
{
  BOOST_ASSERT(block.hasWire());
  const uint8_t* networkPacket = block.wire();
  size_t networkPacketSize = block.size();
  
  uint16_t fragCount = static_cast<uint16_t>(
                         (networkPacketSize / m_maxPayload) +
                         (networkPacketSize % m_maxPayload == 0 ? 0 : 1)
                       );
  PacketArray pa = make_shared<std::vector<Block> >();
  pa->reserve(fragCount);
  SequenceBlock seqBlock = m_seqgen.nextBlock(fragCount);
  
  for (uint16_t fragIndex = 0; fragIndex < fragCount; ++fragIndex) {
    size_t payloadOffset = fragIndex * m_maxPayload;
    const uint8_t* payload = networkPacket + payloadOffset;
    size_t payloadSize = std::min(m_maxPayload, networkPacketSize - payloadOffset);
    
    ndn::EncodingBuffer buffer(m_mtu, 0);
    size_t pktSize = Slicer_encodeFragment(buffer,
      seqBlock[fragIndex], fragIndex, fragCount, payload, payloadSize);
    
    BOOST_ASSERT(pktSize <= m_mtu);
    
    pa->push_back(buffer.block());
  }
  
  return pa;
}

} // namespace ndnlp
} // namespace nfd
