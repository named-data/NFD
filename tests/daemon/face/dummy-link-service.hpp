/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_FACE_DUMMY_LINK_SERVICE_HPP
#define NFD_TESTS_DAEMON_FACE_DUMMY_LINK_SERVICE_HPP

#include "face/link-service.hpp"

namespace nfd {
namespace face {
namespace tests {

enum PacketLoggingFlags : unsigned {
  LogNothing          = 0,      ///< disable packet logging
  LogSentInterests    = 1 << 0, ///< log sent Interest packets
  LogSentData         = 1 << 1, ///< log sent Data packets
  LogSentNacks        = 1 << 2, ///< log sent Nack packets
  LogSentPackets      = LogSentInterests | LogSentData | LogSentNacks, ///< log all sent packets
  LogReceivedPackets  = 1 << 3, ///< log all received link-layer packets
  LogAllPackets       = LogSentPackets | LogReceivedPackets, ///< log all sent and received packets
};

struct RxPacket
{
  Block packet;
  EndpointId endpoint;
};

/** \brief A dummy LinkService that logs all sent and received packets.
 */
class DummyLinkService final : public LinkService
{
public:
  /** \brief Emitted after a network-layer packet is sent through this link service.
   *
   *  The packet type is reported via the argument, whose value will be one of
   *  tlv::Interest, tlv::Data, or lp::tlv::Nack. Signal handlers may retrieve
   *  the packet via `sentInterests.back()`, `sentData.back()`, or `sentNacks.back()`.
   */
  signal::Signal<DummyLinkService, uint32_t> afterSend;

  using LinkService::receiveInterest;
  using LinkService::receiveData;
  using LinkService::receiveNack;

  void
  setPacketLogging(std::underlying_type_t<PacketLoggingFlags> flags)
  {
    m_loggingFlags = static_cast<PacketLoggingFlags>(flags);
  }

private:
  void
  doSendInterest(const Interest& interest, const EndpointId& endpoint) final;

  void
  doSendData(const Data& data, const EndpointId& endpoint) final;

  void
  doSendNack(const lp::Nack& nack, const EndpointId& endpoint) final;

  void
  doReceivePacket(const Block& packet, const EndpointId& endpoint) final;

public:
  std::vector<Interest> sentInterests;
  std::vector<Data> sentData;
  std::vector<lp::Nack> sentNacks;
  std::vector<RxPacket> receivedPackets;

private:
  PacketLoggingFlags m_loggingFlags = LogAllPackets;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_DUMMY_LINK_SERVICE_HPP
