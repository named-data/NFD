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

#include "test-ip.hpp"
#include "test-netif.hpp"

namespace nfd {
namespace face {
namespace tests {

std::ostream&
operator<<(std::ostream& os, AddressFamily family)
{
  switch (family) {
    case AddressFamily::V4:
      return os << "IPv4";
    case AddressFamily::V6:
      return os << "IPv6";
    case AddressFamily::Any:
      return os << "Any";
  }
  return os << '?';
}

std::ostream&
operator<<(std::ostream& os, AddressScope scope)
{
  switch (scope) {
    case AddressScope::Loopback:
      return os << "Loopback";
    case AddressScope::LinkLocal:
      return os << "LinkLocal";
    case AddressScope::Global:
      return os << "Global";
    case AddressScope::Any:
      return os << "Any";
  }
  return os << '?';
}

std::ostream&
operator<<(std::ostream& os, MulticastInterface mcast)
{
  switch (mcast) {
    case MulticastInterface::No:
      return os << "No";
    case MulticastInterface::Yes:
      return os << "Yes";
    case MulticastInterface::Any:
      return os << "Any";
  }
  return os << '?';
}

template<typename E>
static bool
matchTristate(E e, bool b)
{
  return (e == E::Any) ||
         (e == E::Yes && b) ||
         (e == E::No && !b);
}

boost::asio::ip::address
getTestIp(AddressFamily family, AddressScope scope, MulticastInterface mcast)
{
  for (const auto& interface : collectNetworkInterfaces()) {
    if (!interface->isUp() ||
        !matchTristate(mcast, interface->canMulticast())) {
      continue;
    }
    for (const auto& address : interface->getNetworkAddresses()) {
      if (!address.getIp().is_unspecified() &&
          (family == AddressFamily::Any ||
           static_cast<int>(family) == static_cast<int>(address.getFamily())) &&
          (scope == AddressScope::Any ||
           static_cast<int>(scope) == static_cast<int>(address.getScope())) &&
          (scope != AddressScope::Loopback ||
           address.getIp().is_loopback())) {
        return address.getIp();
      }
    }
  }
  return {};
}

} // namespace tests
} // namespace face
} // namespace nfd
