/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "ndnlp-parse.hpp"

namespace nfd {
namespace ndnlp {

void
NdnlpData::wireDecode(const Block& wire)
{
  if (wire.type() != tlv::NdnlpData) {
    throw ParseError("top element is not NdnlpData");
  }
  wire.parse();
  const Block::element_container& elements = wire.elements();
  if (elements.size() < 2) {
    throw ParseError("NdnlpData element has incorrect number of children");
  }

  const Block& sequenceElement = elements.front();
  if (sequenceElement.type() != tlv::NdnlpSequence) {
    throw ParseError("NdnlpSequence element is missing");
  }
  if (sequenceElement.value_size() != sizeof(uint64_t)) {
    throw ParseError("NdnlpSequence element has incorrect length");
  }
  m_seq = be64toh(*reinterpret_cast<const uint64_t*>(&*sequenceElement.value_begin()));

  const Block& payloadElement = elements.back();
  if (payloadElement.type() != tlv::NdnlpPayload) {
    throw ParseError("NdnlpPayload element is missing");
  }
  m_payload = payloadElement;

  if (elements.size() == 2) { // single wire packet
    m_fragIndex = 0;
    m_fragCount = 1;
    return;
  }
  if (elements.size() != 4) {
    throw ParseError("NdnlpData element has incorrect number of children");
  }

  const Block& fragIndexElement = elements.at(1);
  if (fragIndexElement.type() != tlv::NdnlpFragIndex) {
    throw ParseError("NdnlpFragIndex element is missing");
  }
  uint64_t fragIndex = ndn::readNonNegativeInteger(fragIndexElement);
  if (fragIndex > std::numeric_limits<uint16_t>::max()) {
    throw ParseError("NdnlpFragIndex is too large");
  }
  m_fragIndex = static_cast<uint16_t>(fragIndex);

  const Block& fragCountElement = elements.at(2);
  if (fragCountElement.type() != tlv::NdnlpFragCount) {
    throw ParseError("NdnlpFragCount element is missing");
  }
  uint64_t fragCount = ndn::readNonNegativeInteger(fragCountElement);
  if (fragCount > std::numeric_limits<uint16_t>::max()) {
    throw ParseError("NdnlpFragCount is too large");
  }
  m_fragCount = static_cast<uint16_t>(fragCount);

  if (m_fragIndex >= m_fragCount) {
    throw ParseError("NdnlpFragIndex must be less than NdnlpFragCount");
  }
}

} // namespace ndnlp
} // namespace nfd
