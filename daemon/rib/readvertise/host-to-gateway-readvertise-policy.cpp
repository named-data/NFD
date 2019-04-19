/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "host-to-gateway-readvertise-policy.hpp"
#include "fw/scope-prefix.hpp"
#include "mgmt/rib-manager.hpp"

#include <ndn-cxx/security/pib/identity.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

namespace nfd {
namespace rib {

static const name::Component IGNORE_COMPONENT("nrd");
static const time::seconds DEFAULT_REFRESH_INTERVAL = 25_s;

HostToGatewayReadvertisePolicy::HostToGatewayReadvertisePolicy(const ndn::KeyChain& keyChain,
                                                               const ConfigSection& section)
  : m_keyChain(keyChain)
{
  auto interval = section.get_optional<uint64_t>("refresh_interval");
  m_refreshInterval = interval ? time::seconds(*interval) : DEFAULT_REFRESH_INTERVAL;
}

optional<ReadvertiseAction>
HostToGatewayReadvertisePolicy::handleNewRoute(const RibRouteRef& ribRoute) const
{
  auto ribEntryName = ribRoute.entry->getName();
  if (scope_prefix::LOCALHOST.isPrefixOf(ribEntryName) ||
      ribEntryName == RibManager::LOCALHOP_TOP_PREFIX) {
    return nullopt;
  }

  // find out the shortest identity whose name is a prefix of the RIB entry name
  auto prefixToAdvertise = ribEntryName;
  ndn::security::pib::Identity signingIdentity;
  bool isFound = false;

  for (const auto& identity : m_keyChain.getPib().getIdentities()) {
    auto prefix = identity.getName();

    // ignore the identity name's last component if it is "nrd"
    if (!prefix.empty() && IGNORE_COMPONENT == prefix.at(-1)) {
      prefix = prefix.getPrefix(-1);
    }

    if (prefix.isPrefixOf(prefixToAdvertise)) {
      isFound = true;
      prefixToAdvertise = prefix;
      signingIdentity = identity;
    }
  }

  if (isFound) {
    return ReadvertiseAction{prefixToAdvertise, ndn::security::signingByIdentity(signingIdentity)};
  }
  else {
    return nullopt;
  }
}

time::milliseconds
HostToGatewayReadvertisePolicy::getRefreshInterval() const
{
  return m_refreshInterval;
}

} // namespace rib
} // namespace nfd
