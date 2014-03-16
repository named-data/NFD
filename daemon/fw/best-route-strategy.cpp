/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "best-route-strategy.hpp"

namespace nfd {
namespace fw {

const Name BestRouteStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/best-route");

BestRouteStrategy::BestRouteStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}

BestRouteStrategy::~BestRouteStrategy()
{
}

static inline bool
predicate_PitEntry_canForwardTo_NextHop(shared_ptr<pit::Entry> pitEntry,
                                        const fib::NextHop& nexthop)
{
  return pitEntry->canForwardTo(*nexthop.getFace());
}

void
BestRouteStrategy::afterReceiveInterest(const Face& inFace,
                   const Interest& interest,
                   shared_ptr<fib::Entry> fibEntry,
                   shared_ptr<pit::Entry> pitEntry)
{
  if (pitEntry->hasUnexpiredOutRecords()) {
    // not a new Interest, don't forward
    return;
  }

  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  fib::NextHopList::const_iterator it = std::find_if(nexthops.begin(), nexthops.end(),
    bind(&predicate_PitEntry_canForwardTo_NextHop, pitEntry, _1));

  if (it == nexthops.end()) {
    this->rejectPendingInterest(pitEntry);
    return;
  }

  shared_ptr<Face> outFace = it->getFace();
  this->sendInterest(pitEntry, outFace);
}

} // namespace fw
} // namespace nfd
