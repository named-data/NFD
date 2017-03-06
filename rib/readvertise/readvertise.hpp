/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#ifndef NFD_RIB_READVERTISE_READVERTISE_HPP
#define NFD_RIB_READVERTISE_READVERTISE_HPP

#include "readvertise-destination.hpp"
#include "readvertise-policy.hpp"
#include "readvertised-route.hpp"
#include "../rib.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace rib {

/** \brief readvertise a subset of routes to a destination according to a policy
 *
 *  The Readvertise class allows RIB routes to be readvertised to a destination such as a routing
 *  protocol daemon or another NFD-RIB. It monitors the RIB for route additions and removals,
 *  asks the ReadvertisePolicy to make decision on whether to readvertise each new route and what
 *  prefix to readvertise as, and invokes a ReadvertiseDestination to send the commands.
 */
class Readvertise : noncopyable
{

public:
  Readvertise(Rib& rib,
              unique_ptr<ReadvertisePolicy> policy,
              unique_ptr<ReadvertiseDestination> destination);

private:
  void
  afterAddRoute(const RibRouteRef& ribRoute);

  void
  beforeRemoveRoute(const RibRouteRef& ribRoute);

  void
  afterDestinationAvailable();

  void
  afterDestinationUnavailable();

  void
  advertise(ReadvertisedRouteContainer::iterator rrIt);

  void
  withdraw(ReadvertisedRouteContainer::iterator rrIt);

private:
  /** \brief maps from RIB route to readvertised route derived from RIB route(s)
   */
  using RouteRrIndex = std::map<RibRouteRef, ReadvertisedRouteContainer::iterator>;

  static const time::milliseconds RETRY_DELAY_MIN;
  static const time::milliseconds RETRY_DELAY_MAX;

  unique_ptr<ReadvertisePolicy> m_policy;
  unique_ptr<ReadvertiseDestination> m_destination;

  ReadvertisedRouteContainer m_rrs;
  RouteRrIndex m_routeToRr;

  signal::ScopedConnection m_addRouteConn;
  signal::ScopedConnection m_removeRouteConn;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_READVERTISE_READVERTISE_HPP
