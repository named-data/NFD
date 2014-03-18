/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "measurements-entry.hpp"

namespace nfd {
namespace measurements {

Entry::Entry(const Name& name)
  : m_name(name)
  , m_expiry(time::steady_clock::TimePoint::min())
{
}

} // namespace measurements
} // namespace nfd
