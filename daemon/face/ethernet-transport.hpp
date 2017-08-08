/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef NFD_DAEMON_FACE_ETHERNET_TRANSPORT_HPP
#define NFD_DAEMON_FACE_ETHERNET_TRANSPORT_HPP

#include "ethernet-protocol.hpp"
#include "pcap-helper.hpp"
#include "transport.hpp"
#include <ndn-cxx/net/network-interface.hpp>

namespace nfd {
namespace face {

/**
 * @brief Base class for Ethernet-based Transports
 */
class EthernetTransport : public Transport
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  /**
   * @brief Processes the payload of an incoming frame
   * @param payload Pointer to the first byte of data after the Ethernet header
   * @param length Payload length
   * @param sender Sender address
   */
  void
  receivePayload(const uint8_t* payload, size_t length,
                 const ethernet::Address& sender);

protected:
  EthernetTransport(const ndn::net::NetworkInterface& localEndpoint,
                    const ethernet::Address& remoteEndpoint);

  void
  doClose() final;

  bool
  hasRecentlyReceived() const
  {
    return m_hasRecentlyReceived;
  }

  void
  resetRecentlyReceived()
  {
    m_hasRecentlyReceived = false;
  }

private:
  void
  doSend(Transport::Packet&& packet) final;

  /**
   * @brief Sends the specified TLV block on the network wrapped in an Ethernet frame
   */
  void
  sendPacket(const ndn::Block& block);

  void
  asyncRead();

  void
  handleRead(const boost::system::error_code& error);

  void
  handleError(const std::string& errorMessage);

protected:
  boost::asio::posix::stream_descriptor m_socket;
  PcapHelper m_pcap;
  ethernet::Address m_srcAddress;
  ethernet::Address m_destAddress;
  std::string m_interfaceName;

private:
  bool m_hasRecentlyReceived;
#ifdef _DEBUG
  /// number of frames dropped by the kernel, as reported by libpcap
  size_t m_nDropped;
#endif
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_ETHERNET_TRANSPORT_HPP
