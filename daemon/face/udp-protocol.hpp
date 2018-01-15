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

#ifndef NFD_DAEMON_FACE_UDP_PROTOCOL_HPP
#define NFD_DAEMON_FACE_UDP_PROTOCOL_HPP

#include "core/common.hpp"

namespace nfd {
namespace udp {

typedef boost::asio::ip::udp::endpoint Endpoint;

/** \brief computes maximum payload size in a UDP packet
 */
ssize_t
computeMtu(const Endpoint& localEndpoint);

/** \return default IPv4 multicast group: 224.0.23.170:56363
 */
inline Endpoint
getDefaultMulticastGroup()
{
  return {boost::asio::ip::address_v4(0xE00017AA), 56363};
}

/** \return default IPv6 multicast group: [FF02::1234]:56363
 */
inline Endpoint
getDefaultMulticastGroupV6()
{
  return {boost::asio::ip::address_v6::from_string("FF02::1234"), 56363};
}

} // namespace udp
} // namespace nfd

#endif // NFD_DAEMON_FACE_UDP_PROTOCOL_HPP
