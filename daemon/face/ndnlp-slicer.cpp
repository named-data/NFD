/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ndnlp-slicer.hpp"

#include <ndn-cxx/encoding/encoding-buffer.hpp>

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
size_t
Slicer::encodeFragment(ndn::EncodingImpl<T>& blk,
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
    size_t fragCountLength = blk.prependNonNegativeInteger(fragCount);
    totalLength += fragCountLength;
    totalLength += blk.prependVarNumber(fragCountLength);
    totalLength += blk.prependVarNumber(tlv::NdnlpFragCount);

    // NdnlpFragIndex
    size_t fragIndexLength = blk.prependNonNegativeInteger(fragIndex);
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

  // NdnlpData
  totalLength += blk.prependVarNumber(totalLength);
  totalLength += blk.prependVarNumber(tlv::NdnlpData);

  return totalLength;
}

void
Slicer::estimateOverhead()
{
  // estimate fragment size with all header fields at largest possible size
  ndn::EncodingEstimator estimator;
  size_t estimatedSize = this->encodeFragment(estimator,
                                              std::numeric_limits<uint64_t>::max(),
                                              std::numeric_limits<uint16_t>::max() - 1,
                                              std::numeric_limits<uint16_t>::max(),
                                              nullptr, m_mtu);

  size_t overhead = estimatedSize - m_mtu; // minus payload length in estimation
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
  PacketArray pa = make_shared<std::vector<Block>>();
  pa->reserve(fragCount);
  SequenceBlock seqBlock = m_seqgen.nextBlock(fragCount);

  for (uint16_t fragIndex = 0; fragIndex < fragCount; ++fragIndex) {
    size_t payloadOffset = fragIndex * m_maxPayload;
    const uint8_t* payload = networkPacket + payloadOffset;
    size_t payloadSize = std::min(m_maxPayload, networkPacketSize - payloadOffset);

    ndn::EncodingBuffer buffer(m_mtu, 0);
    size_t pktSize = this->encodeFragment(buffer,
      seqBlock[fragIndex], fragIndex, fragCount, payload, payloadSize);

    BOOST_VERIFY(pktSize <= m_mtu);

    pa->push_back(buffer.block());
  }

  return pa;
}

} // namespace ndnlp
} // namespace nfd
