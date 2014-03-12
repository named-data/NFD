/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_AVAILABLE_STRATEGIES_HPP
#define NFD_FW_AVAILABLE_STRATEGIES_HPP

#include "strategy.hpp"

namespace nfd {
namespace fw {

shared_ptr<Strategy>
makeDefaultStrategy(Forwarder& forwarder);

void
installStrategies(Forwarder& forwarder);

} // namespace fw
} // namespace nfd

#endif // NFD_FW_AVAILABLE_STRATEGIES_HPP
