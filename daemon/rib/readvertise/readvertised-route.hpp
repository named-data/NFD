/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#ifndef NFD_DAEMON_RIB_READVERTISE_READVERTISED_ROUTE_HPP
#define NFD_DAEMON_RIB_READVERTISE_READVERTISED_ROUTE_HPP

#include "core/common.hpp"

#include <ndn-cxx/security/signing-info.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <set>

namespace nfd::rib {

/**
 * \brief State of a readvertised route.
 */
class ReadvertisedRoute : noncopyable
{
public:
  ReadvertisedRoute(const Name& prefix, uint64_t cost)
    : prefix(prefix)
    , cost(cost)
  {
  }

  friend bool
  operator<(const ReadvertisedRoute& lhs, const ReadvertisedRoute& rhs)
  {
    return lhs.prefix < rhs.prefix;
  }

public:
  Name prefix; ///< readvertised prefix
  uint64_t cost; ///< cost to reach the prefix
  mutable ndn::security::SigningInfo signer; ///< signer for commands
  mutable size_t nRibRoutes = 0; ///< number of RIB routes that cause the readvertisement
  mutable time::milliseconds retryDelay = 0_ms; ///< retry interval (not used for refresh)
  mutable ndn::scheduler::ScopedEventId retryEvt; ///< retry or refresh event
};

using ReadvertisedRouteContainer = std::set<ReadvertisedRoute>;

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_READVERTISE_READVERTISED_ROUTE_HPP
