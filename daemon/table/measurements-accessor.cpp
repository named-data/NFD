/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "measurements-accessor.hpp"

namespace nfd {

MeasurementsAccessor::MeasurementsAccessor(Measurements& measurements,
                                           Fib& fib, fw::Strategy* strategy)
  : m_measurements(measurements)
  , m_fib(fib)
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

  shared_ptr<fib::Entry> fibEntry = m_fib.findLongestPrefixMatch(*entry);
  BOOST_ASSERT(static_cast<bool>(fibEntry));
  if (&fibEntry->getStrategy() == m_strategy) {
    return entry;
  }
  return shared_ptr<measurements::Entry>();
}

} // namespace nfd
