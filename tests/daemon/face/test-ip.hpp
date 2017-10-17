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

#ifndef NFD_TESTS_DAEMON_FACE_TEST_IP_HPP
#define NFD_TESTS_DAEMON_FACE_TEST_IP_HPP

#include "core/common.hpp"

#include <boost/asio/ip/address.hpp>
#include <ndn-cxx/net/network-address.hpp>

namespace nfd {
namespace face {
namespace tests {

enum class AddressFamily {
  V4 = static_cast<int>(ndn::net::AddressFamily::V4),
  V6 = static_cast<int>(ndn::net::AddressFamily::V6),
  Any
};

std::ostream&
operator<<(std::ostream& os, AddressFamily family);

enum class AddressScope {
  Loopback  = static_cast<int>(ndn::net::AddressScope::HOST),
  LinkLocal = static_cast<int>(ndn::net::AddressScope::LINK),
  Global    = static_cast<int>(ndn::net::AddressScope::GLOBAL),
  Any
};

std::ostream&
operator<<(std::ostream& os, AddressScope scope);

enum class MulticastInterface {
  No,
  Yes,
  Any
};

std::ostream&
operator<<(std::ostream& os, MulticastInterface mcast);

/** \brief Derives IP address type from AddressFamily
 */
template<AddressFamily AF>
struct IpAddressFromFamily;

template<>
struct IpAddressFromFamily<AddressFamily::V4>
{
  using type = boost::asio::ip::address_v4;
};

template<>
struct IpAddressFromFamily<AddressFamily::V6>
{
  using type = boost::asio::ip::address_v6;
};

/** \brief Get an IP address from any available network interface
 *  \param family the desired address family
 *  \param scope the desired address scope
 *  \param mcast specifies if the address can, must, or must not be chosen from a multicast-capable interface
 *  \return an IP address, either boost::asio::ip::address_v4 or boost::asio::ip::address_v6
 *  \retval unspecified address, if no appropriate address is available
 */
boost::asio::ip::address
getTestIp(AddressFamily family = AddressFamily::Any,
          AddressScope scope = AddressScope::Any,
          MulticastInterface mcast = MulticastInterface::Any);

/** \brief Skip the rest of the test case if \p address is unavailable
 *
 *  This macro can be used in conjunction with nfd::tests::getTestIp in a test case. Example:
 *  \code
 *  BOOST_AUTO_TEST_CASE(TestCase)
 *  {
 *    auto ip = getTestIp(AddressFamily::V4);
 *    SKIP_IF_IP_UNAVAILABLE(ip);
 *    // Test something that requires an IPv4 address.
 *  }
 *  \endcode
 */
#define SKIP_IF_IP_UNAVAILABLE(address) \
  do { \
    if ((address).is_unspecified()) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require a valid IP address"); \
      return; \
    } \
  } while (false)

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_TEST_IP_HPP
