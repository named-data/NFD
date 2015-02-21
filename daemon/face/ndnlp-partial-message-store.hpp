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

#ifndef NFD_DAEMON_FACE_NDNLP_PARTIAL_MESSAGE_STORE_HPP
#define NFD_DAEMON_FACE_NDNLP_PARTIAL_MESSAGE_STORE_HPP

#include "ndnlp-data.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace ndnlp {

/** \brief represents a partially received message
 */
class PartialMessage
{
public:
  PartialMessage();

  PartialMessage(const PartialMessage&) = delete;

  PartialMessage&
  operator=(const PartialMessage&) = delete;

  PartialMessage(PartialMessage&&) = default;

  PartialMessage&
  operator=(PartialMessage&&) = default;

  bool
  add(uint16_t fragIndex, uint16_t fragCount, const Block& payload);

  bool
  isComplete() const;

  /** \brief reassemble network layer packet
   *  \pre isComplete() == true
   *  \return whether success, network layer packet
   */
  std::tuple<bool, Block>
  reassemble();

  /** \brief reassemble network layer packet from a single fragment
   *  \pre fragment.fragCount == 1
   *  \return whether success, network layer packet
   */
  static std::tuple<bool, Block>
  reassembleSingle(const NdnlpData& fragment);

public:
  scheduler::ScopedEventId expiry;

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

  /** \brief receive a NdnlpData packet
   *
   *  Reassembly errors will be ignored.
   */
  void
  receive(const NdnlpData& pkt);

  /** \brief fires when network layer packet is received
   */
  signal::Signal<PartialMessageStore, Block> onReceive;

private:
  void
  scheduleCleanup(uint64_t messageIdentifier, PartialMessage& partialMessage);

  void
  cleanup(uint64_t messageIdentifier);

private:
  std::unordered_map<uint64_t, PartialMessage> m_partialMessages;

  time::nanoseconds m_idleDuration;
};

} // namespace ndnlp
} // namespace nfd

#endif // NFD_DAEMON_FACE_NDNLP_PARTIAL_MESSAGE_STORE_HPP
