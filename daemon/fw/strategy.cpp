/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "strategy.hpp"

namespace nfd {

Strategy::Strategy(Forwarder& fw)
  : m_fw(fw)
{
}

Strategy::~Strategy()
{
}


void
Strategy::sendInterest(shared_ptr<pit::Entry> pitEntry,
                       shared_ptr<Face> outFace)
{
  m_fw.onOutgoingInterest(pitEntry, *outFace);
}

void
Strategy::rebuffPendingInterest(shared_ptr<pit::Entry> pitEntry)
{
  m_fw.onInterestRebuff(pitEntry);
}

} // namespace nfd
