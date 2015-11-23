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

#ifndef NFD_TESTS_DAEMON_FACE_NETWORK_INTERFACE_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_NETWORK_INTERFACE_FIXTURE_HPP

#include "core/network-interface.hpp"
#include "face/ethernet-transport.hpp"

#include "test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

class NetworkInterfaceFixture : public nfd::tests::BaseFixture
{
protected:
  NetworkInterfaceFixture()
  {
    for (const auto& netif : listNetworkInterfaces()) {
      if (!netif.isLoopback() && netif.isUp()) {
        try {
          face::EthernetTransport transport(netif, ethernet::getBroadcastAddress());
          m_interfaces.push_back(netif);
        }
        catch (const face::EthernetTransport::Error&) {
          // pass
        }
      }
    }
  }

protected:
  std::vector<NetworkInterfaceInfo> m_interfaces;
};

#define SKIP_IF_NETWORK_INTERFACE_COUNT_LT(n) \
  do {                                        \
    if (this->m_interfaces.size() < (n)) {    \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require " \
                                #n " or more network interfaces");  \
      return;                                 \
    }                                         \
  } while (false)

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_NETWORK_INTERFACE_FIXTURE_HPP
