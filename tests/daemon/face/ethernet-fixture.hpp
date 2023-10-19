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

#ifndef NFD_TESTS_DAEMON_FACE_ETHERNET_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_ETHERNET_FIXTURE_HPP

#include "face/multicast-ethernet-transport.hpp"

#include "tests/daemon/global-io-fixture.hpp"
#include "test-netif.hpp"

namespace nfd::tests {

/**
 * \brief Fixture providing a list of EthernetTransport-capable network interfaces.
 */
class EthernetFixture : public virtual GlobalIoFixture
{
protected:
  EthernetFixture()
  {
    for (const auto& netif : collectNetworkInterfaces()) {
      // similar filtering logic to EthernetFactory::applyMcastConfigToNetif()
      if (netif->isUp() && !netif->isLoopback() && netif->canMulticast()) {
        try {
          face::MulticastEthernetTransport t(*netif, ethernet::getBroadcastAddress(),
                                             ndn::nfd::LINK_TYPE_MULTI_ACCESS);
          netifs.push_back(netif);
        }
        catch (const face::EthernetTransport::Error&) {
          // ignore
        }
      }
    }
  }

protected:
  /**
   * \brief EthernetTransport-capable network interfaces.
   */
  std::vector<shared_ptr<const ndn::net::NetworkInterface>> netifs;
};

} // namespace nfd::tests

#define SKIP_IF_ETHERNET_NETIF_COUNT_LT(n) \
  do { \
    if (this->netifs.size() < (n)) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require " #n \
                                " or more EthernetTransport-capable network interfaces"); \
      return; \
    } \
  } while (false)

#endif // NFD_TESTS_DAEMON_FACE_ETHERNET_FIXTURE_HPP
