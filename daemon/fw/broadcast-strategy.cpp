/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "broadcast-strategy.hpp"

namespace nfd {
namespace fw {

const Name BroadcastStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/broadcast");

BroadcastStrategy::BroadcastStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}

BroadcastStrategy::~BroadcastStrategy()
{
}

void
BroadcastStrategy::afterReceiveInterest(const Face& inFace,
                   const Interest& interest,
                   shared_ptr<fib::Entry> fibEntry,
                   shared_ptr<pit::Entry> pitEntry)
{
  const fib::NextHopList& nexthops = fibEntry->getNextHops();

  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    shared_ptr<Face> outFace = it->getFace();
    if (pitEntry->canForwardTo(*outFace)) {
      this->sendInterest(pitEntry, outFace);
    }
  }

  if (!pitEntry->hasUnexpiredOutRecords()) {
    this->rejectPendingInterest(pitEntry);
  }
}

} // namespace fw
} // namespace nfd
