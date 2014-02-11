/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "strategy-info-host.hpp"

namespace nfd {

void
StrategyInfoHost::clearStrategyInfo()
{
  m_strategyInfo.reset();
}

} // namespace nfd
