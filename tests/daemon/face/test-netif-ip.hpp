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

#ifndef NFD_TESTS_DAEMON_FACE_TEST_NETIF_IP_HPP
#define NFD_TESTS_DAEMON_FACE_TEST_NETIF_IP_HPP

#include "core/common.hpp"
#include <type_traits>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <ndn-cxx/net/network-address.hpp>
#include <ndn-cxx/net/network-interface.hpp>

namespace nfd {
namespace tests {

using ndn::net::AddressFamily;
using ndn::net::NetworkAddress;
using ndn::net::NetworkInterface;

// ---- network interface ----

/** \brief Collect information about network interfaces
 *  \param allowCached if true, previously collected information can be returned
 *  \note This function is blocking if \p allowCached is false or no previous information exists
 *  \throw ndn::net::NetworkMonitor::Error NetworkMonitor::CAP_ENUM is unavailable
 */
std::vector<shared_ptr<const NetworkInterface>>
collectNetworkInterfaces(bool allowCached = true);

template<AddressFamily AF>
bool
isAddressFamily(const NetworkAddress& a)
{
  return a.getFamily() == AF;
}

template<AddressFamily AF>
bool
hasAddressFamily(const NetworkInterface& netif)
{
  return std::any_of(netif.getNetworkAddresses().begin(), netif.getNetworkAddresses().end(),
                     &isAddressFamily<AF>);
}

// ---- IP address ----

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
 *  \tparam F address family, either AddressFamily::V4 or AddressFamily::V6
 *  \param loopback specifies if the address can, must, or must not be a loopback address
 *  \param mcast specifies if the address can, must, or must not be chosen from a multicast-capable interface
 *  \return an IP address, either boost::asio::ip::address_v4 or boost::asio::ip::address_v6
 *  \retval unspecified address, if no appropriate address is available
 */
template<AddressFamily F>
typename IpAddressFromFamily<F>::type
getTestIp(LoopbackAddress loopback = LoopbackAddress::Default,
          MulticastInterface mcast = MulticastInterface::Default);

extern template
IpAddressFromFamily<AddressFamily::V4>::type
getTestIp<AddressFamily::V4>(LoopbackAddress loopback, MulticastInterface mcast);

extern template
IpAddressFromFamily<AddressFamily::V6>::type
getTestIp<AddressFamily::V6>(LoopbackAddress loopback, MulticastInterface mcast);

/** \brief Skip rest of the test case if \p address is unavailable
 *
 *  This macro can be used in conjunction with \p nfd::tests::getTestIp in a test case. Example:
 *  \code
 *  BOOST_AUTO_TEST_CASE(TestCase)
 *  {
 *    auto ip = getTestIp<AddressFamily::V4>();
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
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_TEST_NETIF_IP_HPP
