/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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
LpReliability::observeOutgoing(const std::vector<lp::Packet>& frags)
{
  BOOST_ASSERT(m_options.isEnabled);

  // The sequence number of the first fragment is used to identify the NetPkt.
  lp::Sequence netPktIdentifier = frags.at(0).get<lp::SequenceField>();
  auto& netPkt = m_netPkts.emplace(netPktIdentifier, NetPkt{}).first->second;
  auto unackedFragsIt = m_unackedFrags.begin();
  auto netPktUnackedFragsIt = netPkt.unackedFrags.begin();

  for (const lp::Packet& frag : frags) {
    // Store LpPacket for future retransmissions
    lp::Sequence seq = frag.get<lp::SequenceField>();
    unackedFragsIt = m_unackedFrags.emplace_hint(unackedFragsIt, seq, frag);
    unackedFragsIt->second.rtoTimer =
      scheduler::schedule(m_rto.computeRto(), bind(&LpReliability::onLpPacketLost, this, seq));
    unackedFragsIt->second.sendTime = time::steady_clock::now();
    netPktUnackedFragsIt = netPkt.unackedFrags.insert(netPktUnackedFragsIt, seq);
    if (m_unackedFrags.size() == 1) {
      m_firstUnackedFrag = unackedFragsIt;
    }
  }
}

void
LpReliability::processIncomingPacket(const lp::Packet& pkt)
{
  BOOST_ASSERT(m_options.isEnabled);

  auto now = time::steady_clock::now();

  // Extract and parse Acks
  for (lp::Sequence ackSeq : pkt.list<lp::AckField>()) {
    auto txFrag = m_unackedFrags.find(ackSeq);
    if (txFrag == m_unackedFrags.end()) {
      // Ignore an Ack for an unknown sequence number
      continue;
    }

    // Cancel the RTO timer for the acknowledged fragment
    txFrag->second.rtoTimer.cancel();

    if (txFrag->second.retxCount == 0) {
      // This sequence had no retransmissions, so use it to calculate the RTO
      m_rto.addMeasurement(time::duration_cast<RttEstimator::Duration>(now - txFrag->second.sendTime));
    }

    // Look for Acks with sequence numbers < ackSeq (allowing for wraparound) and consider them lost
    // if a configurable number of Acks containing greater sequence numbers have been received.
    auto lostLpPackets = findLostLpPackets(ackSeq);

    // Remove the fragment from the map of unacknowledged sequences and from its associated network
    // packet (removing the network packet if it has been received in whole by remote host).
    // Potentially increment the start of the window.
    onLpPacketAcknowledged(txFrag, getNetPktByFrag(ackSeq));

    // Resend or fail fragments considered lost. This must be done separately from the above
    // enhanced for loop because onLpPacketLost may delete the fragment from m_unackedFrags.
    for (const lp::Sequence& seq : lostLpPackets) {
      this->onLpPacketLost(seq);
    }
  }

  // If has Fragment field, extract Sequence and add to AckQueue
  if (pkt.has<lp::FragmentField>() && pkt.has<lp::SequenceField>()) {
    m_ackQueue.push(pkt.get<lp::SequenceField>());
    if (!m_isIdleAckTimerRunning) {
      this->startIdleAckTimer();
    }
  }
}

void
LpReliability::piggyback(lp::Packet& pkt, ssize_t mtu)
{
  BOOST_ASSERT(m_options.isEnabled);

  int maxAcks = std::numeric_limits<int>::max();
  if (mtu > 0) {
    // Ack Type (3 octets) + Ack Length (1 octet) + sizeof(lp::Sequence)
    size_t ackSize = 3 + 1 + sizeof(lp::Sequence);
    ndn::EncodingEstimator estimator;
    maxAcks = (mtu - pkt.wireEncode(estimator)) / ackSize;
  }

  ssize_t nAcksInPkt = 0;
  while (!m_ackQueue.empty() && nAcksInPkt < maxAcks) {
    pkt.add<lp::AckField>(m_ackQueue.front());
    m_ackQueue.pop();
    nAcksInPkt++;
  }
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
LpReliability::findLostLpPackets(lp::Sequence ackSeq)
{
  std::vector<lp::Sequence> lostLpPackets;

  for (auto it = m_firstUnackedFrag; ; ++it) {
    if (it == m_unackedFrags.end()) {
      it = m_unackedFrags.begin();
    }

    if (it->first == ackSeq) {
      break;
    }

    auto& unackedFrag = it->second;

    unackedFrag.nGreaterSeqAcks++;

    if (unackedFrag.nGreaterSeqAcks >= m_options.seqNumLossThreshold && !unackedFrag.wasTimedOutBySeq) {
      unackedFrag.wasTimedOutBySeq = true;
      lostLpPackets.push_back(it->first);
    }
  }

  return lostLpPackets;
}

void
LpReliability::onLpPacketLost(lp::Sequence seq)
{
  auto& txFrag = m_unackedFrags.at(seq);
  auto netPktIt = getNetPktByFrag(seq);

  // Check if maximum number of retransmissions exceeded
  if (txFrag.retxCount >= m_options.maxRetx) {
    // Delete all LpPackets of NetPkt from TransmitCache
    lp::Sequence firstSeq = *(netPktIt->second.unackedFrags.begin());
    lp::Sequence lastSeq = *(std::prev(netPktIt->second.unackedFrags.end()));
    if (lastSeq >= firstSeq) { // Normal case: no wraparound
      m_unackedFrags.erase(m_unackedFrags.find(firstSeq), std::next(m_unackedFrags.find(lastSeq)));
    }
    else { // sequence number wraparound
      m_unackedFrags.erase(m_unackedFrags.find(firstSeq), m_unackedFrags.end());
      m_unackedFrags.erase(m_unackedFrags.begin(), std::next(m_unackedFrags.find(lastSeq)));
    }

    m_netPkts.erase(netPktIt);

    ++m_linkService->nRetxExhausted;
  }
  else {
    txFrag.retxCount++;

    // Start RTO timer for this sequence
    txFrag.rtoTimer = scheduler::schedule(m_rto.computeRto(),
                                          bind(&LpReliability::onLpPacketLost, this, seq));

    // Retransmit fragment
    m_linkService->sendLpPacket(lp::Packet(txFrag.pkt));
  }
}

void
LpReliability::onLpPacketAcknowledged(std::map<lp::Sequence, LpReliability::UnackedFrag>::iterator fragIt,
                                      std::map<lp::Sequence, LpReliability::NetPkt>::iterator netPktIt)
{
  lp::Sequence seq = fragIt->first;
  // We need to store the sequence of the window begin in case we are erasing it from m_unackedFrags
  lp::Sequence firstUnackedSeq = m_firstUnackedFrag->first;
  auto nextSeqIt = m_unackedFrags.erase(fragIt);
  netPktIt->second.unackedFrags.erase(seq);

  if (!m_unackedFrags.empty() && firstUnackedSeq == seq) {
    // If "first" fragment in send window (allowing for wraparound), increment window begin
    if (nextSeqIt == m_unackedFrags.end()) {
      m_firstUnackedFrag = m_unackedFrags.begin();
    }
    else {
      m_firstUnackedFrag = nextSeqIt;
    }
  }

  // Check if network-layer packet completely received. If so, delete network packet mapping
  // and increment counter
  if (netPktIt->second.unackedFrags.empty()) {
    if (netPktIt->second.didRetx) {
      ++m_linkService->nRetransmitted;
    }
    else {
      ++m_linkService->nAcknowledged;
    }
    m_netPkts.erase(netPktIt);
  }
}

std::map<lp::Sequence, LpReliability::NetPkt>::iterator
LpReliability::getNetPktByFrag(lp::Sequence seq)
{
  BOOST_ASSERT(!m_netPkts.empty());
  auto it = m_netPkts.lower_bound(seq);
  if (it == m_netPkts.end()) {
    // This can happen because of sequence number wraparound in the middle of a network packet.
    // In this case, the network packet will be at the end of m_netPkts and we will need to
    // decrement the iterator to m_netPkts.end() to the one before it.
    --it;
  }
  return it;
}

LpReliability::UnackedFrag::UnackedFrag(lp::Packet pkt)
  : pkt(std::move(pkt))
  , sendTime(time::steady_clock::now())
  , retxCount(0)
  , nGreaterSeqAcks(0)
  , wasTimedOutBySeq(false)
{
}

LpReliability::NetPkt::NetPkt()
  : didRetx(false)
{
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<LpReliability>& flh)
{
  os << FaceLogHelper<LinkService>(*flh.obj.getLinkService());
  return os;
}

} // namespace face
} // namespace nfd
