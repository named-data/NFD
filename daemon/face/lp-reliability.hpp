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

#ifndef NFD_DAEMON_FACE_LP_RELIABILITY_HPP
#define NFD_DAEMON_FACE_LP_RELIABILITY_HPP

#include "core/rtt-estimator.hpp"
#include "core/scheduler.hpp"

#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/lp/sequence.hpp>

#include <queue>

namespace nfd {
namespace face {

class GenericLinkService;

/** \brief provides for reliable sending and receiving of link-layer packets
 *  \sa https://redmine.named-data.net/projects/nfd/wiki/NDNLPv2
 */
class LpReliability : noncopyable
{
public:
  struct Options
  {
    /** \brief enables link-layer reliability
     */
    bool isEnabled = false;

    /** \brief maximum number of retransmissions for an LpPacket
     */
    size_t maxRetx = 3;

    /** \brief period between sending pending Acks in an IDLE packet
     */
    time::nanoseconds idleAckTimerPeriod = time::milliseconds(5);

    /** \brief a fragment is considered lost if this number of fragments with greater sequence
     *         numbers are acknowledged
     */
    size_t seqNumLossThreshold = 3;
  };

  LpReliability(const Options& options, GenericLinkService* linkService);

  /** \brief signals on Interest dropped by reliability system for exceeding allowed number of retx
   */
  signal::Signal<LpReliability, Interest> onDroppedInterest;

  /** \brief set options for reliability
   */
  void
  setOptions(const Options& options);

  /** \return GenericLinkService that owns this instance
   *
   *  This is only used for logging, and may be nullptr.
   */
  const GenericLinkService*
  getLinkService() const;

  /** \brief observe outgoing fragment(s) of a network packet and store for potential retransmission
   *  \param frags fragments of network packet
   *  \param pkt encapsulated network packet
   *  \param isInterest whether the network packet is an Interest
   */
  void
  handleOutgoing(std::vector<lp::Packet>& frags, lp::Packet&& pkt, bool isInterest);

  /** \brief extract and parse all Acks and add Ack for contained Fragment (if any) to AckQueue
   *  \param pkt incoming LpPacket
   */
  void
  processIncomingPacket(const lp::Packet& pkt);

  /** \brief called by GenericLinkService to attach Acks onto an outgoing LpPacket
   *  \param pkt outgoing LpPacket to attach Acks to
   *  \param mtu MTU of the Transport
   */
  void
  piggyback(lp::Packet& pkt, ssize_t mtu);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  class UnackedFrag;
  class NetPkt;
  using UnackedFrags = std::map<lp::Sequence, UnackedFrag>;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /** \brief assign TxSequence number to a fragment
   *  \param frag fragment to assign TxSequence to
   *  \return assigned TxSequence number
   *  \throw std::length_error assigned TxSequence is equal to the start of the existing window
   */
  lp::Sequence
  assignTxSequence(lp::Packet& frag);

  /** \brief start the idle Ack timer
   *
   * This timer requests an IDLE packet to acknowledge pending fragments not already piggybacked.
   * It is called regularly on a period configured in Options::idleAckTimerPeriod. This allows Acks
   * to be returned to the sender, even if the link goes idle.
   */
  void
  startIdleAckTimer();

  /** \brief cancel the idle Ack timer
   */
  void
  stopIdleAckTimer();

  /** \brief find and mark as lost fragments where a configurable number of Acks
   *         (\p m_options.seqNumLossThreshold) have been received for greater TxSequence numbers
   *  \param ackIt iterator pointing to acknowledged fragment
   *  \return vector containing TxSequences of fragments marked lost by this mechanism
   */
  std::vector<lp::Sequence>
  findLostLpPackets(UnackedFrags::iterator ackIt);

  /** \brief resend (or give up on) a lost fragment
   *  \return vector of the TxSequences of fragments removed due to a network packet being removed
   */
  std::vector<lp::Sequence>
  onLpPacketLost(lp::Sequence txSeq);

  /** \brief remove the fragment with the given sequence number from the map of unacknowledged
   *         fragments, as well as its associated network packet (if any)
   *  \param fragIt iterator to acknowledged fragment
   *
   *  If the given TxSequence marks the beginning of the send window, the window will be incremented.
   *  If the associated network packet has been fully transmitted, it will be removed.
   */
  void
  onLpPacketAcknowledged(UnackedFrags::iterator fragIt);

  /** \brief delete a fragment from UnackedFrags and advance acknowledge window if necessary
   *  \param fragIt iterator to an UnackedFrag, must be dereferencable
   *  \post fragIt is not in m_unackedFrags
   *  \post if was equal to m_firstUnackedFrag,
   *        m_firstUnackedFrag is set to the UnackedFrag after fragIt with consideration of
   *        TxSequence number wraparound, or set to m_unackedFrags.end() if m_unackedFrags is empty
   */
  void
  deleteUnackedFrag(UnackedFrags::iterator fragIt);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /** \brief contains a sent fragment that has not been acknowledged and associated data
   */
  class UnackedFrag
  {
  public:
    explicit
    UnackedFrag(lp::Packet pkt);

  public:
    lp::Packet pkt;
    scheduler::ScopedEventId rtoTimer;
    time::steady_clock::TimePoint sendTime;
    size_t retxCount;
    size_t nGreaterSeqAcks; //!< number of Acks received for sequences greater than this fragment
    shared_ptr<NetPkt> netPkt;
  };

  /** \brief contains a network-layer packet with unacknowledged fragments
   */
  class NetPkt
  {
  public:
    NetPkt(lp::Packet&& pkt, bool isInterest);

  public:
    std::vector<UnackedFrags::iterator> unackedFrags;
    lp::Packet pkt;
    bool isInterest;
    bool didRetx;
  };

public:
  /// TxSequence TLV-TYPE (3 octets) + TxSequence TLV-LENGTH (1 octet) + sizeof(lp::Sequence)
  static constexpr size_t RESERVED_HEADER_SPACE = 3 + 1 + sizeof(lp::Sequence);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  Options m_options;
  GenericLinkService* m_linkService;
  UnackedFrags m_unackedFrags;
  /** An iterator that points to the first unacknowledged fragment in the current window. The window
   *  can wrap around so that the beginning of the window is at a TxSequence greater than other
   *  fragments in the window. When the window is moved past the last item in the iterator, the
   *  first fragment in the map will become the start of the window.
   */
  UnackedFrags::iterator m_firstUnackedFrag;
  std::queue<lp::Sequence> m_ackQueue;
  lp::Sequence m_lastTxSeqNo;
  scheduler::ScopedEventId m_idleAckTimer;
  bool m_isIdleAckTimerRunning;
  RttEstimator m_rto;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_LP_RELIABILITY_HPP
