/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_MULTICAST_ETHERNET_TRANSPORT_HPP
#define NFD_DAEMON_FACE_MULTICAST_ETHERNET_TRANSPORT_HPP

#include "ethernet-transport.hpp"

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * NFD Multicast Ethernet Transport Header
 * ---------------------------------------------------------------------------
 * 本ファイルは、Named Data Networking Forwarding Daemon (NFD) における
 * Ethernet ベースのマルチキャスト通信を提供する
 * MulticastEthernetTransport クラスを定義します。
 *
 * raw Ethernet II フレームを用いて、MAC マルチキャストグループ間で
 * パケットを送受信します。Linux 環境ではインターフェースインデックスを
 * 用いてマルチキャスト参加を行います。
 * ---------------------------------------------------------------------------
 */

namespace nfd::face {

/**
 * @brief A multicast Transport that uses raw Ethernet II frames.
 */
class MulticastEthernetTransport final : public EthernetTransport
{
public:
  /**
   * @brief Creates an Ethernet-based transport for multicast communication.
   */
  MulticastEthernetTransport(const ndn::net::NetworkInterface& localEndpoint,
                             const ethernet::Address& mcastAddress,
                             ndn::nfd::LinkType linkType);

private:
  /**
   * @brief Enables receiving frames addressed to our MAC multicast group.
   */
  void
  joinMulticastGroup();

private:
#if defined(__linux__)
  int m_interfaceIndex;
#endif
};

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_MULTICAST_ETHERNET_TRANSPORT_HPP
