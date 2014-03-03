/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "broadcast-strategy.hpp"

namespace nfd {
namespace fw {

BroadcastStrategy::BroadcastStrategy(Forwarder& forwarder)
  : Strategy(forwarder, "ndn:/localhost/nfd/strategy/broadcast")
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

  bool isPropagated = false;
  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    shared_ptr<Face> outFace = it->getFace();
    if (outFace->getId() != inFace.getId()) {
      this->sendInterest(pitEntry, outFace);
      isPropagated = true;
    }
  }

  if (!isPropagated) {
    this->rejectPendingInterest(pitEntry);
  }
}

} // namespace fw
} // namespace nfd
