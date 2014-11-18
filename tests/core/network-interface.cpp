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

#include "core/network-interface.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreNetworkInterface, BaseFixture)

BOOST_AUTO_TEST_CASE(ListNetworkInterfaces)
{
  std::vector<NetworkInterfaceInfo> netifs;
  BOOST_CHECK_NO_THROW(netifs = listNetworkInterfaces());

  for (const auto& netif : netifs) {
    BOOST_TEST_MESSAGE(netif.index << ": " << netif.name);
    BOOST_TEST_MESSAGE("\tether " << netif.etherAddress);
    for (const auto& address : netif.ipv4Addresses)
      BOOST_TEST_MESSAGE("\tinet  " << address);
    for (const auto& address : netif.ipv6Addresses)
      BOOST_TEST_MESSAGE("\tinet6 " << address);
    BOOST_TEST_MESSAGE("\tloopback  : " << netif.isLoopback());
    BOOST_TEST_MESSAGE("\tmulticast : " << netif.isMulticastCapable());
    BOOST_TEST_MESSAGE("\tup        : " << netif.isUp());
  }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
