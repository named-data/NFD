/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "strategy-choice-entry.hpp"
#include "core/logger.hpp"
#include "fw/strategy.hpp"

NFD_LOG_INIT("StrategyChoiceEntry");

namespace nfd {
namespace strategy_choice {

Entry::Entry(const Name& prefix)
  : m_prefix(prefix)
{
}

void
Entry::setStrategy(shared_ptr<fw::Strategy> strategy)
{
  BOOST_ASSERT(static_cast<bool>(strategy));
  m_strategy = strategy;

  NFD_LOG_INFO("Set strategy " << strategy->getName() << " for " << m_prefix << " prefix");
}

} // namespace strategy_choice
} // namespace nfd
