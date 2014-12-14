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

#include "best-route-strategy.hpp"
#include "broadcast-strategy.hpp"
#include "client-control-strategy.hpp"
#include "ncc-strategy.hpp"
#include "best-route-strategy2.hpp"
#include "access-strategy.hpp"

namespace nfd {
namespace fw {

shared_ptr<Strategy>
makeDefaultStrategy(Forwarder& forwarder)
{
  return make_shared<BestRouteStrategy2>(ref(forwarder));
}

template<typename S>
inline void
installStrategy(Forwarder& forwarder)
{
  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  if (!strategyChoice.hasStrategy(S::STRATEGY_NAME)) {
    strategyChoice.install(make_shared<S>(ref(forwarder)));
  }
}

void
installStrategies(Forwarder& forwarder)
{
  installStrategy<BestRouteStrategy>(forwarder);
  installStrategy<BroadcastStrategy>(forwarder);
  installStrategy<ClientControlStrategy>(forwarder);
  installStrategy<NccStrategy>(forwarder);
  installStrategy<BestRouteStrategy2>(forwarder);
  installStrategy<AccessStrategy>(forwarder);
}

} // namespace fw
} // namespace nfd
