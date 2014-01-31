/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "strategy.hpp"
#include "forwarder.hpp"

namespace nfd {
namespace fw {

Strategy::Strategy(Forwarder& forwarder)
  : m_forwarder(forwarder)
{
}

Strategy::~Strategy()
{
}


void
Strategy::sendInterest(shared_ptr<pit::Entry> pitEntry,
                       shared_ptr<Face> outFace)
{
  m_forwarder.onOutgoingInterest(pitEntry, *outFace);
}

void
Strategy::rebuffPendingInterest(shared_ptr<pit::Entry> pitEntry)
{
  m_forwarder.onInterestRebuff(pitEntry);
}

} // namespace fw
} // namespace nfd
