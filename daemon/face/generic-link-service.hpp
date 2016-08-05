/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "core/common.hpp"
#include "core/logger.hpp"

#include "link-service.hpp"
#include "lp-fragmenter.hpp"
#include "lp-reassembler.hpp"

namespace nfd {
namespace face {

/** \brief counters provided by GenericLinkService
 *  \note The type name 'GenericLinkServiceCounters' is implementation detail.
 *        Use 'GenericLinkService::Counters' in public API.
 */
class GenericLinkServiceCounters : public virtual LinkService::Counters
{
public:
  explicit
  GenericLinkServiceCounters(const LpReassembler& reassembler);

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
};

/** \brief GenericLinkService is a LinkService that implements the NDNLPv2 protocol
 *  \sa http://redmine.named-data.net/projects/nfd/wiki/NDNLPv2
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
  };

  /** \brief counters provided by GenericLinkService
   */
  typedef GenericLinkServiceCounters Counters;

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

  virtual const Counters&
  getCounters() const override;

private: // send path
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

  /** \brief encode local fields from tags onto outgoing LpPacket
   *  \param pkt LpPacket containing a complete network layer packet
   */
  static void
  encodeLocalFields(const ndn::TagHost& netPkt, lp::Packet& lpPacket);

  /** \brief send a complete network layer packet
   *  \param pkt LpPacket containing a complete network layer packet
   */
  void
  sendNetPacket(lp::Packet&& pkt);

  /** \brief assign a sequence number to an LpPacket
   */
  void
  assignSequence(lp::Packet& pkt);

  /** \brief assign consecutive sequence numbers to LpPackets
   */
  void
  assignSequences(std::vector<lp::Packet>& pkts);

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

private:
  Options m_options;
  LpFragmenter m_fragmenter;
  LpReassembler m_reassembler;
  lp::Sequence m_lastSeqNo;
};

inline const GenericLinkService::Options&
GenericLinkService::getOptions() const
{
  return m_options;
}

inline void
GenericLinkService::setOptions(const GenericLinkService::Options& options)
{
  m_options = options;
}

inline const GenericLinkService::Counters&
GenericLinkService::getCounters() const
{
  return *this;
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP
