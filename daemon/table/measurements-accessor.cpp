/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "measurements-accessor.hpp"

namespace nfd {

using fw::Strategy;

MeasurementsAccessor::MeasurementsAccessor(Measurements& measurements,
                                           StrategyChoice& strategyChoice,
                                           Strategy* strategy)
  : m_measurements(measurements)
  , m_strategyChoice(strategyChoice)
  , m_strategy(strategy)
{
}

MeasurementsAccessor::~MeasurementsAccessor()
{
}

shared_ptr<measurements::Entry>
MeasurementsAccessor::filter(const shared_ptr<measurements::Entry>& entry)
{
  if (!static_cast<bool>(entry)) {
    return entry;
  }

  Strategy& effectiveStrategy = m_strategyChoice.findEffectiveStrategy(*entry);
  if (&effectiveStrategy == m_strategy) {
    return entry;
  }
  return shared_ptr<measurements::Entry>();
}

} // namespace nfd
