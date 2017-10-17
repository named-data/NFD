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

#include "test-netif.hpp"
#include "core/global-io.hpp"

#include <ndn-cxx/net/network-monitor.hpp>

namespace nfd {
namespace face {
namespace tests {

std::vector<shared_ptr<const NetworkInterface>>
collectNetworkInterfaces(bool allowCached)
{
  using ndn::net::NetworkMonitor;

  static std::vector<shared_ptr<const NetworkInterface>> cached;
  // cached.empty() indicates there's no cached list of netifs.
  // Although it could also mean a system without any network interface, this situation is rare
  // because the loopback interface is present on almost all systems.

  if (!allowCached || cached.empty()) {
    NetworkMonitor netmon(getGlobalIoService());
    if ((netmon.getCapabilities() & NetworkMonitor::CAP_ENUM) == 0) {
      BOOST_THROW_EXCEPTION(NetworkMonitor::Error("NetworkMonitor::CAP_ENUM is unavailable"));
    }

    netmon.onEnumerationCompleted.connect([] { getGlobalIoService().stop(); });
    getGlobalIoService().run();
    getGlobalIoService().reset();

    cached = netmon.listNetworkInterfaces();
  }

  return cached;
}

} // namespace tests
} // namespace face
} // namespace nfd
