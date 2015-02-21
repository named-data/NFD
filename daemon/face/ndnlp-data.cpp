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

#include "ndnlp-data.hpp"

namespace nfd {
namespace ndnlp {

std::tuple<bool, NdnlpData>
NdnlpData::fromBlock(const Block& wire)
{
  if (wire.type() != tlv::NdnlpData) {
    // top element is not NdnlpData
    return std::make_tuple(false, NdnlpData());
  }
  wire.parse();
  const Block::element_container& elements = wire.elements();
  if (elements.size() < 2) {
    // NdnlpData element has incorrect number of children
    return std::make_tuple(false, NdnlpData());
  }

  NdnlpData parsed;

  const Block& sequenceElement = elements.front();
  if (sequenceElement.type() != tlv::NdnlpSequence) {
    // NdnlpSequence element is missing
    return std::make_tuple(false, NdnlpData());
  }
  if (sequenceElement.value_size() != sizeof(uint64_t)) {
    // NdnlpSequence element has incorrect length
    return std::make_tuple(false, NdnlpData());
  }
  parsed.seq = be64toh(*reinterpret_cast<const uint64_t*>(&*sequenceElement.value_begin()));

  const Block& payloadElement = elements.back();
  if (payloadElement.type() != tlv::NdnlpPayload) {
    // NdnlpPayload element is missing
    return std::make_tuple(false, NdnlpData());
  }
  parsed.payload = payloadElement;

  if (elements.size() == 2) { // single wire packet
    parsed.fragIndex = 0;
    parsed.fragCount = 1;
    return std::make_tuple(true, parsed);
  }
  if (elements.size() != 4) {
    // NdnlpData element has incorrect number of children
    return std::make_tuple(false, NdnlpData());
  }

  const Block& fragIndexElement = elements.at(1);
  if (fragIndexElement.type() != tlv::NdnlpFragIndex) {
    // NdnlpFragIndex element is missing
    return std::make_tuple(false, NdnlpData());
  }
  uint64_t fragIndex = ndn::readNonNegativeInteger(fragIndexElement);
  if (fragIndex > std::numeric_limits<uint16_t>::max()) {
    // NdnlpFragIndex is too large
    return std::make_tuple(false, NdnlpData());
  }
  parsed.fragIndex = static_cast<uint16_t>(fragIndex);

  const Block& fragCountElement = elements.at(2);
  if (fragCountElement.type() != tlv::NdnlpFragCount) {
    // NdnlpFragCount element is missing
    return std::make_tuple(false, NdnlpData());
  }
  uint64_t fragCount = ndn::readNonNegativeInteger(fragCountElement);
  if (fragCount > std::numeric_limits<uint16_t>::max()) {
    // NdnlpFragCount is too large
    return std::make_tuple(false, NdnlpData());
  }
  parsed.fragCount = static_cast<uint16_t>(fragCount);

  if (parsed.fragIndex >= parsed.fragCount) {
    // NdnlpFragIndex must be less than NdnlpFragCount
    return std::make_tuple(false, NdnlpData());
  }

  return std::make_tuple(true, parsed);
}

} // namespace ndnlp
} // namespace nfd
