/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "best-route-strategy.hpp"
#include "broadcast-strategy.hpp"
#include "client-control-strategy.hpp"
#include "ncc-strategy.hpp"

namespace nfd {
namespace fw {

shared_ptr<Strategy>
makeDefaultStrategy(Forwarder& forwarder)
{
  return make_shared<BestRouteStrategy>(boost::ref(forwarder));
}

template<typename S>
inline void
installStrategy(Forwarder& forwarder)
{
  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  if (!strategyChoice.hasStrategy(S::STRATEGY_NAME)) {
    strategyChoice.install(make_shared<S>(boost::ref(forwarder)));
  }
}

void
installStrategies(Forwarder& forwarder)
{
  installStrategy<BestRouteStrategy>(forwarder);
  installStrategy<BroadcastStrategy>(forwarder);
  installStrategy<ClientControlStrategy>(forwarder);
  installStrategy<NccStrategy>(forwarder);
}

} // namespace fw
} // namespace nfd
