/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "generic-link-service.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <cmath>

namespace nfd {
namespace face {

NFD_LOG_INIT("GenericLinkService");

constexpr uint32_t DEFAULT_CONGESTION_THRESHOLD_DIVISOR = 2;

GenericLinkService::Options::Options()
  : allowLocalFields(false)
  , allowFragmentation(false)
  , allowReassembly(false)
  , allowCongestionMarking(false)
  , baseCongestionMarkingInterval(time::milliseconds(100)) // Interval from RFC 8289 (CoDel)
  , defaultCongestionThreshold(65536) // This default value works well for a queue capacity of 200KiB
{
}

GenericLinkService::GenericLinkService(const GenericLinkService::Options& options)
  : m_options(options)
  , m_fragmenter(m_options.fragmenterOptions, this)
  , m_reassembler(m_options.reassemblerOptions, this)
  , m_reliability(m_options.reliabilityOptions, this)
  , m_lastSeqNo(-2)
  , m_nextMarkTime(time::steady_clock::TimePoint::max())
  , m_lastMarkTime(time::steady_clock::TimePoint::min())
  , m_nMarkedSinceInMarkingState(0)
{
  m_reassembler.beforeTimeout.connect(bind([this] { ++this->nReassemblyTimeouts; }));
  m_reliability.onDroppedInterest.connect([this] (const Interest& i) { this->notifyDroppedInterest(i); });
  nReassembling.observe(&m_reassembler);
}

void
GenericLinkService::setOptions(const GenericLinkService::Options& options)
{
  m_options = options;
  m_fragmenter.setOptions(m_options.fragmenterOptions);
  m_reassembler.setOptions(m_options.reassemblerOptions);
  m_reliability.setOptions(m_options.reliabilityOptions);
}

void
GenericLinkService::requestIdlePacket()
{
  // No need to request Acks to attach to this packet from LpReliability, as they are already
  // attached in sendLpPacket
  this->sendLpPacket({});
}

void
GenericLinkService::sendLpPacket(lp::Packet&& pkt)
{
  const ssize_t mtu = this->getTransport()->getMtu();

  if (m_options.reliabilityOptions.isEnabled) {
    m_reliability.piggyback(pkt, mtu);
  }

  if (m_options.allowCongestionMarking) {
    checkCongestionLevel(pkt);
  }

  Transport::Packet tp(pkt.wireEncode());
  if (mtu != MTU_UNLIMITED && tp.packet.size() > static_cast<size_t>(mtu)) {
    ++this->nOutOverMtu;
    NFD_LOG_FACE_WARN("attempted to send packet over MTU limit");
    return;
  }
  this->sendPacket(std::move(tp));
}

void
GenericLinkService::doSendInterest(const Interest& interest)
{
  lp::Packet lpPacket(interest.wireEncode());

  encodeLpFields(interest, lpPacket);

  this->sendNetPacket(std::move(lpPacket), true);
}

void
GenericLinkService::doSendData(const Data& data)
{
  lp::Packet lpPacket(data.wireEncode());

  encodeLpFields(data, lpPacket);

  this->sendNetPacket(std::move(lpPacket), false);
}

void
GenericLinkService::doSendNack(const lp::Nack& nack)
{
  lp::Packet lpPacket(nack.getInterest().wireEncode());
  lpPacket.add<lp::NackField>(nack.getHeader());

  encodeLpFields(nack, lpPacket);

  this->sendNetPacket(std::move(lpPacket), false);
}

void
GenericLinkService::encodeLpFields(const ndn::PacketBase& netPkt, lp::Packet& lpPacket)
{
  if (m_options.allowLocalFields) {
    shared_ptr<lp::IncomingFaceIdTag> incomingFaceIdTag = netPkt.getTag<lp::IncomingFaceIdTag>();
    if (incomingFaceIdTag != nullptr) {
      lpPacket.add<lp::IncomingFaceIdField>(*incomingFaceIdTag);
    }
  }

  shared_ptr<lp::CongestionMarkTag> congestionMarkTag = netPkt.getTag<lp::CongestionMarkTag>();
  if (congestionMarkTag != nullptr) {
    lpPacket.add<lp::CongestionMarkField>(*congestionMarkTag);
  }
}

void
GenericLinkService::sendNetPacket(lp::Packet&& pkt, bool isInterest)
{
  std::vector<lp::Packet> frags;
  ssize_t mtu = this->getTransport()->getMtu();

  // Make space for feature fields in fragments
  if (m_options.reliabilityOptions.isEnabled && mtu != MTU_UNLIMITED) {
    mtu -= LpReliability::RESERVED_HEADER_SPACE;
  }

  if (m_options.allowCongestionMarking && mtu != MTU_UNLIMITED) {
    mtu -= CONGESTION_MARK_SIZE;
  }

  BOOST_ASSERT(mtu == MTU_UNLIMITED || mtu > 0);

  if (m_options.allowFragmentation && mtu != MTU_UNLIMITED) {
    bool isOk = false;
    std::tie(isOk, frags) = m_fragmenter.fragmentPacket(pkt, mtu);
    if (!isOk) {
      // fragmentation failed (warning is logged by LpFragmenter)
      ++this->nFragmentationErrors;
      return;
    }
  }
  else {
    if (m_options.reliabilityOptions.isEnabled) {
      frags.push_back(pkt);
    }
    else {
      frags.push_back(std::move(pkt));
    }
  }

  if (frags.size() == 1) {
    // even if indexed fragmentation is enabled, the fragmenter should not
    // fragment the packet if it can fit in MTU
    BOOST_ASSERT(!frags.front().has<lp::FragIndexField>());
    BOOST_ASSERT(!frags.front().has<lp::FragCountField>());
  }

  // Only assign sequences to fragments if packet contains more than 1 fragment
  if (frags.size() > 1) {
    // Assign sequences to all fragments
    this->assignSequences(frags);
  }

  if (m_options.reliabilityOptions.isEnabled && frags.front().has<lp::FragmentField>()) {
    m_reliability.handleOutgoing(frags, std::move(pkt), isInterest);
  }

  for (lp::Packet& frag : frags) {
    this->sendLpPacket(std::move(frag));
  }
}

void
GenericLinkService::assignSequence(lp::Packet& pkt)
{
  pkt.set<lp::SequenceField>(++m_lastSeqNo);
}

void
GenericLinkService::assignSequences(std::vector<lp::Packet>& pkts)
{
  std::for_each(pkts.begin(), pkts.end(), bind(&GenericLinkService::assignSequence, this, _1));
}

void
GenericLinkService::checkCongestionLevel(lp::Packet& pkt)
{
  ssize_t sendQueueLength = getTransport()->getSendQueueLength();
  // This operation requires that the transport supports retrieving current send queue length
  if (sendQueueLength < 0) {
    return;
  }

  // To avoid overflowing the queue, set the congestion threshold to at least half of the send
  // queue capacity.
  size_t congestionThreshold = m_options.defaultCongestionThreshold;
  if (getTransport()->getSendQueueCapacity() >= 0) {
    congestionThreshold = std::min(congestionThreshold,
                                   static_cast<size_t>(getTransport()->getSendQueueCapacity()) /
                                                       DEFAULT_CONGESTION_THRESHOLD_DIVISOR);
  }

  if (sendQueueLength > 0) {
    NFD_LOG_FACE_TRACE("txqlen=" << sendQueueLength << " threshold=" << congestionThreshold <<
                       " capacity=" << getTransport()->getSendQueueCapacity());
  }

  if (static_cast<size_t>(sendQueueLength) > congestionThreshold) { // Send queue is congested
    const auto now = time::steady_clock::now();
    if (now >= m_nextMarkTime || now >= m_lastMarkTime + m_options.baseCongestionMarkingInterval) {
      // Mark at most one initial packet per baseCongestionMarkingInterval
      if (m_nMarkedSinceInMarkingState == 0) {
        m_nextMarkTime = now;
      }

      // Time to mark packet
      pkt.set<lp::CongestionMarkField>(1);
      ++nCongestionMarked;
      NFD_LOG_FACE_DEBUG("LpPacket was marked as congested");

      ++m_nMarkedSinceInMarkingState;
      // Decrease the marking interval by the inverse of the square root of the number of packets
      // marked in this incident of congestion
      m_nextMarkTime += time::nanoseconds(static_cast<time::nanoseconds::rep>(
                                            m_options.baseCongestionMarkingInterval.count() /
                                            std::sqrt(m_nMarkedSinceInMarkingState)));
      m_lastMarkTime = now;
    }
  }
  else if (m_nextMarkTime != time::steady_clock::TimePoint::max()) {
    // Congestion incident has ended, so reset
    NFD_LOG_FACE_DEBUG("Send queue length dropped below congestion threshold");
    m_nextMarkTime = time::steady_clock::TimePoint::max();
    m_nMarkedSinceInMarkingState = 0;
  }
}

void
GenericLinkService::doReceivePacket(Transport::Packet&& packet)
{
  try {
    lp::Packet pkt(packet.packet);

    if (m_options.reliabilityOptions.isEnabled) {
      m_reliability.processIncomingPacket(pkt);
    }

    if (!pkt.has<lp::FragmentField>()) {
      NFD_LOG_FACE_TRACE("received IDLE packet: DROP");
      return;
    }

    if ((pkt.has<lp::FragIndexField>() || pkt.has<lp::FragCountField>()) &&
        !m_options.allowReassembly) {
      NFD_LOG_FACE_WARN("received fragment, but reassembly disabled: DROP");
      return;
    }

    bool isReassembled = false;
    Block netPkt;
    lp::Packet firstPkt;
    std::tie(isReassembled, netPkt, firstPkt) = m_reassembler.receiveFragment(packet.remoteEndpoint,
                                                                              pkt);
    if (isReassembled) {
      this->decodeNetPacket(netPkt, firstPkt);
    }
  }
  catch (const tlv::Error& e) {
    ++this->nInLpInvalid;
    NFD_LOG_FACE_WARN("packet parse error (" << e.what() << "): DROP");
  }
}

void
GenericLinkService::decodeNetPacket(const Block& netPkt, const lp::Packet& firstPkt)
{
  try {
    switch (netPkt.type()) {
      case tlv::Interest:
        if (firstPkt.has<lp::NackField>()) {
          this->decodeNack(netPkt, firstPkt);
        }
        else {
          this->decodeInterest(netPkt, firstPkt);
        }
        break;
      case tlv::Data:
        this->decodeData(netPkt, firstPkt);
        break;
      default:
        ++this->nInNetInvalid;
        NFD_LOG_FACE_WARN("unrecognized network-layer packet TLV-TYPE " << netPkt.type() << ": DROP");
        return;
    }
  }
  catch (const tlv::Error& e) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("packet parse error (" << e.what() << "): DROP");
  }
}

void
GenericLinkService::decodeInterest(const Block& netPkt, const lp::Packet& firstPkt)
{
  BOOST_ASSERT(netPkt.type() == tlv::Interest);
  BOOST_ASSERT(!firstPkt.has<lp::NackField>());

  // forwarding expects Interest to be created with make_shared
  auto interest = make_shared<Interest>(netPkt);

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    if (m_options.allowLocalFields) {
      interest->setTag(make_shared<lp::NextHopFaceIdTag>(firstPkt.get<lp::NextHopFaceIdField>()));
    }
    else {
      NFD_LOG_FACE_WARN("received NextHopFaceId, but local fields disabled: DROP");
      return;
    }
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received CachePolicy with Interest: DROP");
    return;
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  if (firstPkt.has<lp::CongestionMarkField>()) {
    interest->setTag(make_shared<lp::CongestionMarkTag>(firstPkt.get<lp::CongestionMarkField>()));
  }

  this->receiveInterest(*interest);
}

void
GenericLinkService::decodeData(const Block& netPkt, const lp::Packet& firstPkt)
{
  BOOST_ASSERT(netPkt.type() == tlv::Data);

  // forwarding expects Data to be created with make_shared
  auto data = make_shared<Data>(netPkt);

  if (firstPkt.has<lp::NackField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received Nack with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received NextHopFaceId with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    // CachePolicy is unprivileged and does not require allowLocalFields option.
    // In case of an invalid CachePolicyType, get<lp::CachePolicyField> will throw,
    // so it's unnecessary to check here.
    data->setTag(make_shared<lp::CachePolicyTag>(firstPkt.get<lp::CachePolicyField>()));
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  if (firstPkt.has<lp::CongestionMarkField>()) {
    data->setTag(make_shared<lp::CongestionMarkTag>(firstPkt.get<lp::CongestionMarkField>()));
  }

  this->receiveData(*data);
}

void
GenericLinkService::decodeNack(const Block& netPkt, const lp::Packet& firstPkt)
{
  BOOST_ASSERT(netPkt.type() == tlv::Interest);
  BOOST_ASSERT(firstPkt.has<lp::NackField>());

  lp::Nack nack((Interest(netPkt)));
  nack.setHeader(firstPkt.get<lp::NackField>());

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received NextHopFaceId with Nack: DROP");
    return;
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received CachePolicy with Nack: DROP");
    return;
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  if (firstPkt.has<lp::CongestionMarkField>()) {
    nack.setTag(make_shared<lp::CongestionMarkTag>(firstPkt.get<lp::CongestionMarkField>()));
  }

  this->receiveNack(nack);
}

} // namespace face
} // namespace nfd
