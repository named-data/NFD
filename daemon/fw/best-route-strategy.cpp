/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "best-route-strategy.hpp"

namespace nfd {

BestRouteStrategy::BestRouteStrategy(Forwarder& fw)
  : Strategy(fw)
{
}

BestRouteStrategy::~BestRouteStrategy()
{
}

void
BestRouteStrategy::afterReceiveInterest(const Face& inFace,
                   const Interest& interest,
                   shared_ptr<fib::Entry> fibEntry,
                   shared_ptr<pit::Entry> pitEntry,
                   pit::InRecordCollection::iterator pitInRecord)
{
  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  if (nexthops.size() == 0) {
    this->rebuffPendingInterest(pitEntry);
    return;
  }
  
  shared_ptr<Face> outFace = nexthops.begin()->getFace();
  this->sendInterest(pitEntry, outFace);
}

} // namespace nfd
