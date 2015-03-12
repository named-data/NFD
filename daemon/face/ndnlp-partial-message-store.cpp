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

#include "ndnlp-partial-message-store.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace ndnlp {

NFD_LOG_INIT("NdnlpPartialMessageStore");

PartialMessage::PartialMessage()
  : m_fragCount(0)
  , m_received(0)
  , m_totalLength(0)
{
}

bool
PartialMessage::add(uint16_t fragIndex, uint16_t fragCount, const Block& payload)
{
  if (m_received == 0) { // first packet
    m_fragCount = fragCount;
    m_payloads.resize(fragCount);
  }

  if (m_fragCount != fragCount || fragIndex >= m_fragCount) {
    return false;
  }

  if (!m_payloads[fragIndex].empty()) { // duplicate
    return false;
  }

  m_payloads[fragIndex] = payload;
  ++m_received;
  m_totalLength += payload.value_size();
  return true;
}

bool
PartialMessage::isComplete() const
{
  return m_received == m_fragCount;
}

std::tuple<bool, Block>
PartialMessage::reassemble()
{
  BOOST_ASSERT(this->isComplete());

  ndn::BufferPtr buffer = make_shared<ndn::Buffer>(m_totalLength);
  ndn::Buffer::iterator buf = buffer->begin();
  for (const Block& payload : m_payloads) {
    buf = std::copy(payload.value_begin(), payload.value_end(), buf);
  }
  BOOST_ASSERT(buf == buffer->end());

  return Block::fromBuffer(buffer, 0);
}

std::tuple<bool, Block>
PartialMessage::reassembleSingle(const NdnlpData& fragment)
{
  BOOST_ASSERT(fragment.fragCount == 1);

  try {
    return std::make_tuple(true, fragment.payload.blockFromValue());
  }
  catch (tlv::Error&) {
    return std::make_tuple(false, Block());
  }
}

PartialMessageStore::PartialMessageStore(const time::nanoseconds& idleDuration)
  : m_idleDuration(idleDuration)
{
}

void
PartialMessageStore::receive(const NdnlpData& pkt)
{
  bool isReassembled = false;
  Block reassembled;

  if (pkt.fragCount == 1) { // single fragment
    std::tie(isReassembled, reassembled) = PartialMessage::reassembleSingle(pkt);
  }
  else {
    uint64_t messageIdentifier = pkt.seq - pkt.fragIndex;
    PartialMessage& pm = m_partialMessages[messageIdentifier];
    this->scheduleCleanup(messageIdentifier, pm);

    pm.add(pkt.fragIndex, pkt.fragCount, pkt.payload);

    if (pm.isComplete()) {
      std::tie(isReassembled, reassembled) = pm.reassemble();
      m_partialMessages.erase(messageIdentifier);
    }
    else {
      return;
    }
  }

  if (!isReassembled) {
    NFD_LOG_TRACE(pkt.seq << " reassemble error");
    return;
  }

  NFD_LOG_TRACE(pkt.seq << " deliver");
  this->onReceive(reassembled);
}

void
PartialMessageStore::scheduleCleanup(uint64_t messageIdentifier,
                                     PartialMessage& partialMessage)
{
  partialMessage.expiry = scheduler::schedule(m_idleDuration,
    bind(&PartialMessageStore::cleanup, this, messageIdentifier));
}

void
PartialMessageStore::cleanup(uint64_t messageIdentifier)
{
  NFD_LOG_TRACE(messageIdentifier << " cleanup");
  m_partialMessages.erase(messageIdentifier);
}

} // namespace ndnlp
} // namespace nfd
