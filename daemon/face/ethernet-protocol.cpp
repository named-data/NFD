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

#include "ethernet-protocol.hpp"

#include <arpa/inet.h>  // for ntohs()

namespace nfd {
namespace ethernet {

std::pair<const ether_header*, std::string>
checkFrameHeader(const uint8_t* packet, size_t length,
                 const Address& localAddr, const Address& destAddr)
{
  if (length < HDR_LEN + MIN_DATA_LEN)
    return {nullptr, "Received frame too short: " + to_string(length) + " bytes"};

  const ether_header* eh = reinterpret_cast<const ether_header*>(packet);

  // in some cases VLAN-tagged frames may survive the BPF filter,
  // make sure we do not process those frames (see #3348)
  if (ntohs(eh->ether_type) != ETHERTYPE_NDN)
    return {nullptr, "Received frame with wrong ethertype: " + to_string(ntohs(eh->ether_type))};

#ifdef _DEBUG
  Address shost(eh->ether_shost);
  if (shost == localAddr)
    return {nullptr, "Received frame sent by this host"};

  Address dhost(eh->ether_dhost);
  if (dhost != destAddr)
    return {nullptr, "Received frame addressed to another host or multicast group: " + dhost.toString()};
#endif

  return {eh, ""};
}

} // namespace ethernet
} // namespace nfd
