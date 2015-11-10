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

#include "lp-fragmenter.hpp"
#include "link-service.hpp"
#include <ndn-cxx/encoding/tlv.hpp>

namespace nfd {
namespace face {

NFD_LOG_INIT("LpFragmenter");

static_assert(lp::tlv::LpPacket < 253, "LpPacket TLV-TYPE must fit in 1 octet");
static_assert(lp::tlv::Sequence < 253, "Sequence TLV-TYPE must fit in 1 octet");
static_assert(lp::tlv::FragIndex < 253, "FragIndex TLV-TYPE must fit in 1 octet");
static_assert(lp::tlv::FragCount < 253, "FragCount TLV-TYPE must fit in 1 octet");
static_assert(lp::tlv::Fragment < 253, "Fragment TLV-TYPE must fit in 1 octet");

/** \brief maximum overhead on a single fragment,
 *         not counting other NDNLPv2 headers
 */
static const size_t MAX_SINGLE_FRAG_OVERHEAD =
  1 + 9 + // LpPacket TLV-TYPE and TLV-LENGTH
  1 + 1 + 8 + // Sequence TLV
  1 + 9; // Fragment TLV-TYPE and TLV-LENGTH

/** \brief maximum overhead of adding fragmentation to payload,
 *         not counting other NDNLPv2 headers
 */
static const size_t MAX_FRAG_OVERHEAD =
  1 + 9 + // LpPacket TLV-TYPE and TLV-LENGTH
  1 + 1 + 8 + // Sequence TLV
  1 + 1 + 8 + // FragIndex TLV
  1 + 1 + 8 + // FragCount TLV
  1 + 9; // Fragment TLV-TYPE and TLV-LENGTH

LpFragmenter::Options::Options()
  : nMaxFragments(400)
{
}

LpFragmenter::LpFragmenter(const LpFragmenter::Options& options, const LinkService* linkService)
  : m_options(options)
  , m_linkService(linkService)
{
}

void
LpFragmenter::setOptions(const Options& options)
{
  m_options = options;
}

const LinkService*
LpFragmenter::getLinkService() const
{
  return m_linkService;
}

std::tuple<bool, std::vector<lp::Packet>>
LpFragmenter::fragmentPacket(const lp::Packet& packet, size_t mtu)
{
  BOOST_ASSERT(packet.has<lp::FragmentField>());
  BOOST_ASSERT(!packet.has<lp::FragIndexField>());
  BOOST_ASSERT(!packet.has<lp::FragCountField>());

  if (MAX_SINGLE_FRAG_OVERHEAD + packet.wireEncode().size() <= mtu) {
    // fast path: fragmentation not needed
    // To qualify for fast path, the packet must have space for adding a sequence number,
    // because another NDNLPv2 feature may require the sequence number.
    return std::make_tuple(true, std::vector<lp::Packet>{packet});
  }

  ndn::Buffer::const_iterator netPktBegin, netPktEnd;
  std::tie(netPktBegin, netPktEnd) = packet.get<lp::FragmentField>();
  size_t netPktSize = std::distance(netPktBegin, netPktEnd);

  // compute size of other NDNLPv2 headers to be placed on the first fragment
  size_t firstHeaderSize = 0;
  const Block& packetWire = packet.wireEncode();
  if (packetWire.type() == lp::tlv::LpPacket) {
    for (const Block& element : packetWire.elements()) {
      if (element.type() != lp::tlv::Fragment) {
        firstHeaderSize += element.size();
      }
    }
  }

  // compute payload size
  if (MAX_FRAG_OVERHEAD + firstHeaderSize + 1 > mtu) { // 1-octet fragment
    NFD_LOG_FACE_WARN("fragmentation error, MTU too small for first fragment: DROP");
    return std::make_tuple(false, std::vector<lp::Packet>{});
  }
  size_t firstPayloadSize = std::min(netPktSize, mtu - firstHeaderSize - MAX_FRAG_OVERHEAD);
  size_t payloadSize = mtu - MAX_FRAG_OVERHEAD;
  size_t fragCount = 1 + ((netPktSize - firstPayloadSize) / payloadSize) +
                     ((netPktSize - firstPayloadSize) % payloadSize != 0);

  // compute FragCount
  if (fragCount > m_options.nMaxFragments) {
    NFD_LOG_FACE_WARN("fragmentation error, FragCount over limit: DROP");
    return std::make_pair(false, std::vector<lp::Packet>{});
  }

  // populate fragments
  std::vector<lp::Packet> frags(fragCount);
  frags.front() = packet; // copy input packet to preserve other NDNLPv2 fields
  size_t fragIndex = 0;
  auto fragBegin = netPktBegin,
       fragEnd = fragBegin + firstPayloadSize;
  while (fragBegin < netPktEnd) {
    lp::Packet& frag = frags[fragIndex];
    frag.add<lp::FragIndexField>(fragIndex);
    frag.add<lp::FragCountField>(fragCount);
    frag.set<lp::FragmentField>(std::make_pair(fragBegin, fragEnd));
    BOOST_ASSERT(frag.wireEncode().size() <= mtu);

    ++fragIndex;
    fragBegin = fragEnd;
    fragEnd = std::min(netPktEnd, fragBegin + payloadSize);
  }
  BOOST_ASSERT(fragIndex == fragCount);

  return std::make_pair(true, frags);
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<LpFragmenter>& flh)
{
  if (flh.obj.getLinkService() == nullptr) {
    os << "[id=0,local=unknown,remote=unknown] ";
  }
  else {
    os << FaceLogHelper<LinkService>(*flh.obj.getLinkService());
  }
  return os;
}

} // namespace face
} // namespace nfd
