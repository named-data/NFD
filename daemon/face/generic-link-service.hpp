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

#ifndef NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP
#define NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP

#include "link-service.hpp"
#include "lp-fragmenter.hpp"
#include "lp-reassembler.hpp"
#include "lp-reliability.hpp"

namespace nfd {
namespace face {

/** \brief counters provided by GenericLinkService
 *  \note The type name 'GenericLinkServiceCounters' is implementation detail.
 *        Use 'GenericLinkService::Counters' in public API.
 */
class GenericLinkServiceCounters : public virtual LinkService::Counters
{
public:
  /** \brief count of failed fragmentations
   */
  PacketCounter nFragmentationErrors;

  /** \brief count of outgoing LpPackets dropped due to exceeding MTU limit
   *
   *  If this counter is non-zero, the operator should enable fragmentation.
   */
  PacketCounter nOutOverMtu;

  /** \brief count of invalid LpPackets dropped before reassembly
   */
  PacketCounter nInLpInvalid;

  /** \brief count of network-layer packets currently being reassembled
   */
  SizeCounter<LpReassembler> nReassembling;

  /** \brief count of dropped partial network-layer packets due to reassembly timeout
   */
  PacketCounter nReassemblyTimeouts;

  /** \brief count of invalid reassembled network-layer packets dropped
   */
  PacketCounter nInNetInvalid;

  /** \brief count of network-layer packets that did not require retransmission of a fragment
   */
  PacketCounter nAcknowledged;

  /** \brief count of network-layer packets that had at least one fragment retransmitted, but were
   *         eventually received in full
   */
  PacketCounter nRetransmitted;

  /** \brief count of network-layer packets dropped because a fragment reached the maximum number
   *         of retransmissions
   */
  PacketCounter nRetxExhausted;

  /** \brief count of outgoing LpPackets that were marked with congestion marks
   */
  PacketCounter nCongestionMarked;
};

/** \brief GenericLinkService is a LinkService that implements the NDNLPv2 protocol
 *  \sa https://redmine.named-data.net/projects/nfd/wiki/NDNLPv2
 */
class GenericLinkService : public LinkService
                         , protected virtual GenericLinkServiceCounters
{
public:
  /** \brief Options that control the behavior of GenericLinkService
   */
  class Options
  {
  public:
    Options();

  public:
    /** \brief enables encoding of IncomingFaceId, and decoding of NextHopFaceId and CachePolicy
     */
    bool allowLocalFields;

    /** \brief enables fragmentation
     */
    bool allowFragmentation;

    /** \brief options for fragmentation
     */
    LpFragmenter::Options fragmenterOptions;

    /** \brief enables reassembly
     */
    bool allowReassembly;

    /** \brief options for reassembly
     */
    LpReassembler::Options reassemblerOptions;

    /** \brief options for reliability
     */
    LpReliability::Options reliabilityOptions;

    /** \brief enables send queue congestion detection and marking
     */
    bool allowCongestionMarking;

    /** \brief starting value for congestion marking interval
     */
    time::nanoseconds baseCongestionMarkingInterval;

    /** \brief default congestion threshold in bytes
     */
    size_t defaultCongestionThreshold;

    /** \brief enables self-learning forwarding support
     */
    bool allowSelfLearning;
  };

  /** \brief counters provided by GenericLinkService
   */
  using Counters = GenericLinkServiceCounters;

  explicit
  GenericLinkService(const Options& options = Options());

  /** \brief get Options used by GenericLinkService
   */
  const Options&
  getOptions() const;

  /** \brief sets Options used by GenericLinkService
   */
  void
  setOptions(const Options& options);

  const Counters&
  getCounters() const override;

PROTECTED_WITH_TESTS_ELSE_PRIVATE: // send path
  /** \brief request an IDLE packet to transmit pending service fields
   */
  void
  requestIdlePacket();

  /** \brief send an LpPacket fragment
   *  \param pkt LpPacket to send
   */
  void
  sendLpPacket(lp::Packet&& pkt);

  /** \brief send Interest
   */
  void
  doSendInterest(const Interest& interest) override;

  /** \brief send Data
   */
  void
  doSendData(const Data& data) override;

  /** \brief send Nack
   */
  void
  doSendNack(const ndn::lp::Nack& nack) override;

private: // send path
  /** \brief encode link protocol fields from tags onto an outgoing LpPacket
   *  \param netPkt network-layer packet to extract tags from
   *  \param lpPacket LpPacket to add link protocol fields to
   */
  void
  encodeLpFields(const ndn::PacketBase& netPkt, lp::Packet& lpPacket);

  /** \brief send a complete network layer packet
   *  \param pkt LpPacket containing a complete network layer packet
   *  \param isInterest whether the network layer packet is an Interest
   */
  void
  sendNetPacket(lp::Packet&& pkt, bool isInterest);

  /** \brief assign a sequence number to an LpPacket
   */
  void
  assignSequence(lp::Packet& pkt);

  /** \brief assign consecutive sequence numbers to LpPackets
   */
  void
  assignSequences(std::vector<lp::Packet>& pkts);

  /** \brief if the send queue is found to be congested, add a congestion mark to the packet
   *         according to CoDel
   *  \sa https://tools.ietf.org/html/rfc8289
   */
  void
  checkCongestionLevel(lp::Packet& pkt);

private: // receive path
  /** \brief receive Packet from Transport
   */
  void
  doReceivePacket(Transport::Packet&& packet) override;

  /** \brief decode incoming network-layer packet
   *  \param netPkt reassembled network-layer packet
   *  \param firstPkt LpPacket of first fragment
   *
   *  If decoding is successful, a receive signal is emitted;
   *  otherwise, a warning is logged.
   */
  void
  decodeNetPacket(const Block& netPkt, const lp::Packet& firstPkt);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Interest
   *  \param firstPkt LpPacket of first fragment; must not have Nack field
   *
   *  If decoding is successful, receiveInterest signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeInterest(const Block& netPkt, const lp::Packet& firstPkt);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Data
   *  \param firstPkt LpPacket of first fragment
   *
   *  If decoding is successful, receiveData signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeData(const Block& netPkt, const lp::Packet& firstPkt);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Interest
   *  \param firstPkt LpPacket of first fragment; must have Nack field
   *
   *  If decoding is successful, receiveNack signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeNack(const Block& netPkt, const lp::Packet& firstPkt);

PROTECTED_WITH_TESTS_ELSE_PRIVATE:
  Options m_options;
  LpFragmenter m_fragmenter;
  LpReassembler m_reassembler;
  LpReliability m_reliability;
  lp::Sequence m_lastSeqNo;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /// CongestionMark TLV-TYPE (3 octets) + CongestionMark TLV-LENGTH (1 octet) + sizeof(uint64_t)
  static constexpr size_t CONGESTION_MARK_SIZE = 3 + 1 + sizeof(uint64_t);
  /// Time to mark next packet due to send queue congestion
  time::steady_clock::TimePoint m_nextMarkTime;
  /// Time last packet was marked
  time::steady_clock::TimePoint m_lastMarkTime;
  /// number of marked packets in the current incident of congestion
  size_t m_nMarkedSinceInMarkingState;

  friend class LpReliability;
};

inline const GenericLinkService::Options&
GenericLinkService::getOptions() const
{
  return m_options;
}

inline const GenericLinkService::Counters&
GenericLinkService::getCounters() const
{
  return *this;
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP
