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

#ifndef NFD_TESTS_DAEMON_FACE_TEST_IP_HPP
#define NFD_TESTS_DAEMON_FACE_TEST_IP_HPP

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>

#define SKIP_IF_IP_UNAVAILABLE(address) \
  do { \
    if ((address).is_unspecified()) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require a valid IP address"); \
      return; \
    } \
  } while (false)

namespace nfd {
namespace tests {

enum class LoopbackAddress {
  No,
  Yes,
  DontCare,
  Default = Yes
};

enum class MulticastInterface {
  No,
  Yes,
  DontCare,
  Default = DontCare
};

/** \brief get an IP address for test purposes from any available network interface
 *  \tparam A the address type, either boost::asio::ip::address_v4 or boost::asio::ip::address_v6
 *  \param loopback specifies if the address can, must, or must not be a loopback address
 *  \param mcast specifies if the address can, must, or must not be chosen from a multicast-capable interface
 *  \return an IP address
 *  \retval default-constructed A, if no appropriate address is available
 */
template<typename A>
A
getTestIp(LoopbackAddress loopback = LoopbackAddress::Default,
          MulticastInterface mcast = MulticastInterface::Default);

template<>
boost::asio::ip::address_v4
getTestIp(LoopbackAddress loopback, MulticastInterface mcast);

template<>
boost::asio::ip::address_v6
getTestIp(LoopbackAddress loopback, MulticastInterface mcast);

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_TEST_IP_HPP
