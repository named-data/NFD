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

#include "lp-reliability.hpp"
#include "generic-link-service.hpp"
#include "transport.hpp"

namespace nfd {
namespace face {

LpReliability::LpReliability(const LpReliability::Options& options, GenericLinkService* linkService)
  : m_options(options)
  , m_linkService(linkService)
  , m_firstUnackedFrag(m_unackedFrags.begin())
  , m_lastTxSeqNo(-1) // set to "-1" to start TxSequence numbers at 0
  , m_isIdleAckTimerRunning(false)
{
  BOOST_ASSERT(m_linkService != nullptr);

  BOOST_ASSERT(m_options.idleAckTimerPeriod > time::nanoseconds::zero());
}

void
LpReliability::setOptions(const Options& options)
{
  BOOST_ASSERT(options.idleAckTimerPeriod > time::nanoseconds::zero());

  if (m_options.isEnabled && !options.isEnabled) {
    this->stopIdleAckTimer();
  }

  m_options = options;
}

const GenericLinkService*
LpReliability::getLinkService() const
{
  return m_linkService;
}

void
LpReliability::handleOutgoing(std::vector<lp::Packet>& frags, lp::Packet&& pkt, bool isInterest)
{
  BOOST_ASSERT(m_options.isEnabled);

  auto unackedFragsIt = m_unackedFrags.begin();
  auto sendTime = time::steady_clock::now();

  auto netPkt = make_shared<NetPkt>(std::move(pkt), isInterest);
  netPkt->unackedFrags.reserve(frags.size());

  for (lp::Packet& frag : frags) {
    // Assign TxSequence number
    lp::Sequence txSeq = assignTxSequence(frag);

    // Store LpPacket for future retransmissions
    unackedFragsIt = m_unackedFrags.emplace_hint(unackedFragsIt,
                                                 std::piecewise_construct,
                                                 std::forward_as_tuple(txSeq),
                                                 std::forward_as_tuple(frag));
    unackedFragsIt->second.sendTime = sendTime;
    unackedFragsIt->second.rtoTimer =
      scheduler::schedule(m_rto.computeRto(), bind(&LpReliability::onLpPacketLost, this, txSeq));
    unackedFragsIt->second.netPkt = netPkt;

    if (m_unackedFrags.size() == 1) {
      m_firstUnackedFrag = m_unackedFrags.begin();
    }

    // Add to associated NetPkt
    netPkt->unackedFrags.push_back(unackedFragsIt);
  }
}

void
LpReliability::processIncomingPacket(const lp::Packet& pkt)
{
  BOOST_ASSERT(m_options.isEnabled);

  auto now = time::steady_clock::now();

  // Extract and parse Acks
  for (lp::Sequence ackSeq : pkt.list<lp::AckField>()) {
    auto fragIt = m_unackedFrags.find(ackSeq);
    if (fragIt == m_unackedFrags.end()) {
      // Ignore an Ack for an unknown TxSequence number
      continue;
    }
    auto& frag = fragIt->second;

    // Cancel the RTO timer for the acknowledged fragment
    frag.rtoTimer.cancel();

    if (frag.retxCount == 0) {
      // This sequence had no retransmissions, so use it to calculate the RTO
      m_rto.addMeasurement(time::duration_cast<RttEstimator::Duration>(now - frag.sendTime));
    }

    // Look for frags with TxSequence numbers < ackSeq (allowing for wraparound) and consider them
    // lost if a configurable number of Acks containing greater TxSequence numbers have been
    // received.
    auto lostLpPackets = findLostLpPackets(fragIt);

    // Remove the fragment from the map of unacknowledged fragments and from its associated network
    // packet. Potentially increment the start of the window.
    onLpPacketAcknowledged(fragIt);

    // This set contains TxSequences that have been removed by onLpPacketLost below because they
    // were part of a network packet that was removed due to a fragment exceeding retx, as well as
    // any other TxSequences removed by onLpPacketLost. This prevents onLpPacketLost from being
    // called later for an invalid iterator.
    std::set<lp::Sequence> removedLpPackets;

    // Resend or fail fragments considered lost. Potentially increment the start of the window.
    for (lp::Sequence txSeq : lostLpPackets) {
      if (removedLpPackets.find(txSeq) == removedLpPackets.end()) {
        auto removedThisTxSeq = this->onLpPacketLost(txSeq);
        for (auto removedTxSeq : removedThisTxSeq) {
          removedLpPackets.insert(removedTxSeq);
        }
      }
    }
  }

  // If packet has Fragment and TxSequence fields, extract TxSequence and add to AckQueue
  if (pkt.has<lp::FragmentField>() && pkt.has<lp::TxSequenceField>()) {
    m_ackQueue.push(pkt.get<lp::TxSequenceField>());
    if (!m_isIdleAckTimerRunning) {
      this->startIdleAckTimer();
    }
  }
}

void
LpReliability::piggyback(lp::Packet& pkt, ssize_t mtu)
{
  BOOST_ASSERT(m_options.isEnabled);
  BOOST_ASSERT(pkt.wireEncode().type() == lp::tlv::LpPacket);

  // up to 2 extra octets reserved for potential TLV-LENGTH size increases
  ssize_t pktSize = pkt.wireEncode().size();
  ssize_t reservedSpace = tlv::sizeOfVarNumber(ndn::MAX_NDN_PACKET_SIZE) -
                          tlv::sizeOfVarNumber(pktSize);
  ssize_t remainingSpace = (mtu == MTU_UNLIMITED ? ndn::MAX_NDN_PACKET_SIZE : mtu) - reservedSpace;
  remainingSpace -= pktSize;

  while (!m_ackQueue.empty()) {
    lp::Sequence ackSeq = m_ackQueue.front();
    // Ack size = Ack TLV-TYPE (3 octets) + TLV-LENGTH (1 octet) + uint64_t (8 octets)
    const ssize_t ackSize = tlv::sizeOfVarNumber(lp::tlv::Ack) +
                            tlv::sizeOfVarNumber(sizeof(lp::Sequence)) +
                            sizeof(lp::Sequence);

    if (ackSize > remainingSpace) {
      break;
    }

    pkt.add<lp::AckField>(ackSeq);
    m_ackQueue.pop();
    remainingSpace -= ackSize;
  }
}

lp::Sequence
LpReliability::assignTxSequence(lp::Packet& frag)
{
  lp::Sequence txSeq = ++m_lastTxSeqNo;
  frag.set<lp::TxSequenceField>(txSeq);
  if (m_unackedFrags.size() > 0 && m_lastTxSeqNo == m_firstUnackedFrag->first) {
    BOOST_THROW_EXCEPTION(std::length_error("TxSequence range exceeded"));
  }
  return m_lastTxSeqNo;
}

void
LpReliability::startIdleAckTimer()
{
  BOOST_ASSERT(!m_isIdleAckTimerRunning);
  m_isIdleAckTimerRunning = true;

  m_idleAckTimer = scheduler::schedule(m_options.idleAckTimerPeriod, [this] {
    while (!m_ackQueue.empty()) {
      m_linkService->requestIdlePacket();
    }

    m_isIdleAckTimerRunning = false;
  });
}

void
LpReliability::stopIdleAckTimer()
{
  m_idleAckTimer.cancel();
  m_isIdleAckTimerRunning = false;
}

std::vector<lp::Sequence>
LpReliability::findLostLpPackets(LpReliability::UnackedFrags::iterator ackIt)
{
  std::vector<lp::Sequence> lostLpPackets;

  for (auto it = m_firstUnackedFrag; ; ++it) {
    if (it == m_unackedFrags.end()) {
      it = m_unackedFrags.begin();
    }

    if (it->first == ackIt->first) {
      break;
    }

    auto& unackedFrag = it->second;
    unackedFrag.nGreaterSeqAcks++;

    if (unackedFrag.nGreaterSeqAcks >= m_options.seqNumLossThreshold) {
      lostLpPackets.push_back(it->first);
    }
  }

  return lostLpPackets;
}

std::vector<lp::Sequence>
LpReliability::onLpPacketLost(lp::Sequence txSeq)
{
  BOOST_ASSERT(m_unackedFrags.count(txSeq) > 0);
  auto txSeqIt = m_unackedFrags.find(txSeq);

  auto& txFrag = txSeqIt->second;
  txFrag.rtoTimer.cancel();
  auto netPkt = txFrag.netPkt;
  std::vector<lp::Sequence> removedThisTxSeq;

  // Check if maximum number of retransmissions exceeded
  if (txFrag.retxCount >= m_options.maxRetx) {
    // Delete all LpPackets of NetPkt from m_unackedFrags (except this one)
    for (size_t i = 0; i < netPkt->unackedFrags.size(); i++) {
      if (netPkt->unackedFrags[i] != txSeqIt) {
        removedThisTxSeq.push_back(netPkt->unackedFrags[i]->first);
        deleteUnackedFrag(netPkt->unackedFrags[i]);
      }
    }

    ++m_linkService->nRetxExhausted;

    // Notify strategy of dropped Interest (if any)
    if (netPkt->isInterest) {
      BOOST_ASSERT(netPkt->pkt.has<lp::FragmentField>());
      ndn::Buffer::const_iterator fragBegin, fragEnd;
      std::tie(fragBegin, fragEnd) = netPkt->pkt.get<lp::FragmentField>();
      Block frag(&*fragBegin, std::distance(fragBegin, fragEnd));
      onDroppedInterest(Interest(frag));
    }

    removedThisTxSeq.push_back(txSeqIt->first);
    deleteUnackedFrag(txSeqIt);
  }
  else {
    // Assign new TxSequence
    lp::Sequence newTxSeq = assignTxSequence(txFrag.pkt);
    netPkt->didRetx = true;

    // Move fragment to new TxSequence mapping
    auto newTxFragIt = m_unackedFrags.emplace_hint(
      m_firstUnackedFrag != m_unackedFrags.end() && m_firstUnackedFrag->first > newTxSeq
        ? m_firstUnackedFrag
        : m_unackedFrags.end(),
      std::piecewise_construct,
      std::forward_as_tuple(newTxSeq),
      std::forward_as_tuple(txFrag.pkt));
    auto& newTxFrag = newTxFragIt->second;
    newTxFrag.retxCount = txFrag.retxCount + 1;
    newTxFrag.netPkt = netPkt;

    // Update associated NetPkt
    auto fragInNetPkt = std::find(netPkt->unackedFrags.begin(), netPkt->unackedFrags.end(), txSeqIt);
    BOOST_ASSERT(fragInNetPkt != netPkt->unackedFrags.end());
    *fragInNetPkt = newTxFragIt;

    removedThisTxSeq.push_back(txSeqIt->first);
    deleteUnackedFrag(txSeqIt);

    // Retransmit fragment
    m_linkService->sendLpPacket(lp::Packet(newTxFrag.pkt));

    // Start RTO timer for this sequence
    newTxFrag.rtoTimer = scheduler::schedule(m_rto.computeRto(),
                                          bind(&LpReliability::onLpPacketLost, this, newTxSeq));
  }

  return removedThisTxSeq;
}

void
LpReliability::onLpPacketAcknowledged(UnackedFrags::iterator fragIt)
{
  auto netPkt = fragIt->second.netPkt;

  // Remove from NetPkt unacked fragment list
  auto fragInNetPkt = std::find(netPkt->unackedFrags.begin(), netPkt->unackedFrags.end(), fragIt);
  BOOST_ASSERT(fragInNetPkt != netPkt->unackedFrags.end());
  *fragInNetPkt = netPkt->unackedFrags.back();
  netPkt->unackedFrags.pop_back();

  // Check if network-layer packet completely received. If so, increment counters
  if (netPkt->unackedFrags.empty()) {
    if (netPkt->didRetx) {
      ++m_linkService->nRetransmitted;
    }
    else {
      ++m_linkService->nAcknowledged;
    }
  }

  deleteUnackedFrag(fragIt);
}

void
LpReliability::deleteUnackedFrag(UnackedFrags::iterator fragIt)
{
  lp::Sequence firstUnackedTxSeq = m_firstUnackedFrag->first;
  lp::Sequence currentTxSeq = fragIt->first;
  auto nextFragIt = m_unackedFrags.erase(fragIt);

  if (!m_unackedFrags.empty() && firstUnackedTxSeq == currentTxSeq) {
    // If "first" fragment in send window (allowing for wraparound), increment window begin
    if (nextFragIt == m_unackedFrags.end()) {
      m_firstUnackedFrag = m_unackedFrags.begin();
    }
    else {
      m_firstUnackedFrag = nextFragIt;
    }
  }
  else if (m_unackedFrags.empty()) {
    m_firstUnackedFrag = m_unackedFrags.end();
  }
}

LpReliability::UnackedFrag::UnackedFrag(lp::Packet pkt)
  : pkt(std::move(pkt))
  , sendTime(time::steady_clock::now())
  , retxCount(0)
  , nGreaterSeqAcks(0)
{
}

LpReliability::NetPkt::NetPkt(lp::Packet&& pkt, bool isInterest)
  : pkt(std::move(pkt))
  , isInterest(isInterest)
  , didRetx(false)
{
}

} // namespace face
} // namespace nfd
