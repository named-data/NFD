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

#ifndef NFD_DAEMON_FACE_NDNLP_PARTIAL_MESSAGE_STORE_HPP
#define NFD_DAEMON_FACE_NDNLP_PARTIAL_MESSAGE_STORE_HPP

#include "ndnlp-parse.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace ndnlp {

/** \brief represents a partially received message
 */
class PartialMessage : noncopyable
{
public:
  PartialMessage();

  bool
  add(uint16_t fragIndex, uint16_t fragCount, const Block& payload);

  bool
  isComplete() const;

  /** \brief reassemble network layer packet
   *
   *  isComplete() must be true before calling this method
   *
   *  \exception ndn::Block::Error packet is malformated
   *  \return network layer packet
   */
  Block
  reassemble();

public:
  EventId m_expiry;

private:
  size_t m_fragCount;
  size_t m_received;
  std::vector<Block> m_payloads;
  size_t m_totalLength;
};

/** \brief provides reassembly feature at receiver
 */
class PartialMessageStore : noncopyable
{
public:
  explicit
  PartialMessageStore(const time::nanoseconds& idleDuration = time::milliseconds(100));

  virtual
  ~PartialMessageStore();

  /** \brief receive a NdnlpData packet
   *
   *  \exception ParseError NDNLP packet is malformated
   *  \exception ndn::Block::Error network layer packet is malformated
   */
  void
  receiveNdnlpData(const Block& pkt);

  /// fires when network layer packet is received
  EventEmitter<Block> onReceive;

private:
  void
  scheduleCleanup(uint64_t messageIdentifier, shared_ptr<PartialMessage> partialMessage);

  void
  cleanup(uint64_t messageIdentifier);

private:
  std::map<uint64_t, shared_ptr<PartialMessage> > m_partialMessages;

  time::nanoseconds m_idleDuration;
};

} // namespace ndnlp
} // namespace nfd

#endif // NFD_DAEMON_FACE_NDNLP_PARTIAL_MESSAGE_STORE_HPP
