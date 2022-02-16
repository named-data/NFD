/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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
#include "common/global.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT(LpReliability);

LpReliability::LpReliability(const LpReliability::Options& options, GenericLinkService* linkService)
  : m_options(options)
  , m_linkService(linkService)
  , m_firstUnackedFrag(m_unackedFrags.begin())
  , m_lastTxSeqNo(-1) // set to "-1" to start TxSequence numbers at 0
{
  BOOST_ASSERT(m_linkService != nullptr);
  BOOST_ASSERT(m_options.idleAckTimerPeriod > 0_ns);
}

void
LpReliability::setOptions(const Options& options)
{
  BOOST_ASSERT(options.idleAckTimerPeriod > 0_ns);

  if (m_options.isEnabled && !options.isEnabled) {
    m_idleAckTimer.cancel();
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
    // Non-IDLE packets are required to have assigned Sequence numbers with LpReliability enabled
    BOOST_ASSERT(frag.has<lp::SequenceField>());

    // Assign TxSequence number
    lp::Sequence txSeq = assignTxSequence(frag);

    // Store LpPacket for future retransmissions
    unackedFragsIt = m_unackedFrags.emplace_hint(unackedFragsIt,
                                                 std::piecewise_construct,
                                                 std::forward_as_tuple(txSeq),
                                                 std::forward_as_tuple(frag));
    unackedFragsIt->second.sendTime = sendTime;
    auto rto = m_rttEst.getEstimatedRto();
    lp::Sequence seq = frag.get<lp::SequenceField>();
    NFD_LOG_FACE_TRACE("transmitting seq=" << seq << ", txseq=" << txSeq << ", rto=" <<
                       time::duration_cast<time::milliseconds>(rto).count() << "ms");
    unackedFragsIt->second.rtoTimer = getScheduler().schedule(rto, [=] {
      onLpPacketLost(txSeq, true);
    });
    unackedFragsIt->second.netPkt = netPkt;

    if (m_unackedFrags.size() == 1) {
      m_firstUnackedFrag = m_unackedFrags.begin();
    }

    // Add to associated NetPkt
    netPkt->unackedFrags.push_back(unackedFragsIt);
  }
}

bool
LpReliability::processIncomingPacket(const lp::Packet& pkt)
{
  BOOST_ASSERT(m_options.isEnabled);

  bool isDuplicate = false;
  auto now = time::steady_clock::now();

  // Extract and parse Acks
  for (lp::Sequence ackTxSeq : pkt.list<lp::AckField>()) {
    auto fragIt = m_unackedFrags.find(ackTxSeq);
    if (fragIt == m_unackedFrags.end()) {
      // Ignore an Ack for an unknown TxSequence number
      NFD_LOG_FACE_DEBUG("received ack for unknown txseq=" << ackTxSeq);
      continue;
    }
    auto& frag = fragIt->second;

    // Cancel the RTO timer for the acknowledged fragment
    frag.rtoTimer.cancel();

    if (frag.retxCount == 0) {
      NFD_LOG_FACE_TRACE("received ack for seq=" << frag.pkt.get<lp::SequenceField>() << ", txseq=" <<
                         ackTxSeq << ", retx=0, rtt=" <<
                         time::duration_cast<time::milliseconds>(now - frag.sendTime).count() << "ms");
      // This sequence had no retransmissions, so use it to estimate the RTO
      m_rttEst.addMeasurement(now - frag.sendTime);
    }
    else {
      NFD_LOG_FACE_TRACE("received ack for seq=" << frag.pkt.get<lp::SequenceField>() << ", txseq=" <<
                         ackTxSeq << ", retx=" << frag.retxCount);
    }

    // Look for frags with TxSequence numbers < ackTxSeq (allowing for wraparound) and consider
    // them lost if a configurable number of Acks containing greater TxSequence numbers have been
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
        auto removedTxSeqs = onLpPacketLost(txSeq, false);
        for (auto removedTxSeq : removedTxSeqs) {
          removedLpPackets.insert(removedTxSeq);
        }
      }
    }
  }

  // If packet has Fragment and TxSequence fields, extract TxSequence and add to AckQueue
  if (pkt.has<lp::FragmentField>() && pkt.has<lp::TxSequenceField>()) {
    NFD_LOG_FACE_TRACE("queueing ack for remote txseq=" << pkt.get<lp::TxSequenceField>());
    m_ackQueue.push(pkt.get<lp::TxSequenceField>());

    // Check for received frames with duplicate Sequences
    if (pkt.has<lp::SequenceField>()) {
      lp::Sequence pktSequence = pkt.get<lp::SequenceField>();
      isDuplicate = m_recentRecvSeqs.count(pktSequence) > 0;
      // Check for recent received Sequences to remove
      auto now = time::steady_clock::now();
      auto rto = m_rttEst.getEstimatedRto();
      while (!m_recentRecvSeqsQueue.empty() &&
             now > m_recentRecvSeqs[m_recentRecvSeqsQueue.front()] + rto) {
        m_recentRecvSeqs.erase(m_recentRecvSeqsQueue.front());
        m_recentRecvSeqsQueue.pop();
      }
      m_recentRecvSeqs.emplace(pktSequence, now);
      m_recentRecvSeqsQueue.push(pktSequence);
    }

    startIdleAckTimer();
  }

  return !isDuplicate;
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
    lp::Sequence ackTxSeq = m_ackQueue.front();
    // Ack size = Ack TLV-TYPE (3 octets) + TLV-LENGTH (1 octet) + lp::Sequence (8 octets)
    const ssize_t ackSize = tlv::sizeOfVarNumber(lp::tlv::Ack) +
                            tlv::sizeOfVarNumber(sizeof(lp::Sequence)) +
                            sizeof(lp::Sequence);

    if (ackSize > remainingSpace) {
      break;
    }

    NFD_LOG_FACE_TRACE("piggybacking ack for remote txseq=" << ackTxSeq);

    pkt.add<lp::AckField>(ackTxSeq);
    m_ackQueue.pop();
    remainingSpace -= ackSize;
  }
}

lp::Sequence
LpReliability::assignTxSequence(lp::Packet& frag)
{
  lp::Sequence txSeq = ++m_lastTxSeqNo;
  frag.set<lp::TxSequenceField>(txSeq);
  if (!m_unackedFrags.empty() && m_lastTxSeqNo == m_firstUnackedFrag->first) {
    NDN_THROW(std::length_error("TxSequence range exceeded"));
  }
  return m_lastTxSeqNo;
}

void
LpReliability::startIdleAckTimer()
{
  if (m_idleAckTimer) {
    // timer is already running, do nothing
    return;
  }

  m_idleAckTimer = getScheduler().schedule(m_options.idleAckTimerPeriod, [this] {
    while (!m_ackQueue.empty()) {
      m_linkService->requestIdlePacket();
    }
  });
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
    NFD_LOG_FACE_TRACE("received ack=" << ackIt->first << " before=" << it->first <<
                       ", before count=" << unackedFrag.nGreaterSeqAcks);

    if (unackedFrag.nGreaterSeqAcks >= m_options.seqNumLossThreshold) {
      lostLpPackets.push_back(it->first);
    }
  }

  return lostLpPackets;
}

std::vector<lp::Sequence>
LpReliability::onLpPacketLost(lp::Sequence txSeq, bool isTimeout)
{
  BOOST_ASSERT(m_unackedFrags.count(txSeq) > 0);
  auto txSeqIt = m_unackedFrags.find(txSeq);

  auto& txFrag = txSeqIt->second;
  txFrag.rtoTimer.cancel();
  auto netPkt = txFrag.netPkt;
  std::vector<lp::Sequence> removedThisTxSeq;
  lp::Sequence seq = txFrag.pkt.get<lp::SequenceField>();

  if (isTimeout) {
    NFD_LOG_FACE_TRACE("rto timer expired for seq=" << seq << ", txseq=" << txSeq);
  }
  else { // lost due to out-of-order TxSeqs
    NFD_LOG_FACE_TRACE("seq=" << seq << ", txseq=" << txSeq <<
                       " considered lost from acks for more recent txseqs");
  }

  // Check if maximum number of retransmissions exceeded
  if (txFrag.retxCount >= m_options.maxRetx) {
    NFD_LOG_FACE_DEBUG("seq=" << seq << " exceeded allowed retransmissions: DROP");
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
      auto frag = netPkt->pkt.get<lp::FragmentField>();
      onDroppedInterest(Interest(Block({frag.first, frag.second})));
    }

    // Delete this LpPacket from m_unackedFrags
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

    auto rto = m_rttEst.getEstimatedRto();
    NFD_LOG_FACE_TRACE("retransmitting seq=" << seq << ", txseq=" << newTxSeq << ", retx=" <<
                       txFrag.retxCount << ", rto=" <<
                       time::duration_cast<time::milliseconds>(rto).count() << "ms");

    // Start RTO timer for this sequence
    newTxFrag.rtoTimer = getScheduler().schedule(rto, [=] {
      onLpPacketLost(newTxSeq, true);
    });
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

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<LpReliability>& flh)
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
