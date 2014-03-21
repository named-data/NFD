/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "client-control-strategy.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("ClientControlStrategy");

const Name ClientControlStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/client-control");

ClientControlStrategy::ClientControlStrategy(Forwarder& forwarder, const Name& name)
  : BestRouteStrategy(forwarder, name)
{
}

ClientControlStrategy::~ClientControlStrategy()
{
}

void
ClientControlStrategy::afterReceiveInterest(const Face& inFace,
                                            const Interest& interest,
                                            shared_ptr<fib::Entry> fibEntry,
                                            shared_ptr<pit::Entry> pitEntry)
{
  // Strategy needn't check whether LocalControlHeader-NextHopFaceId is enabled.
  // LocalFace does this check.
  if (!interest.getLocalControlHeader().hasNextHopFaceId()) {
    this->BestRouteStrategy::afterReceiveInterest(inFace, interest, fibEntry, pitEntry);
    return;
  }

  FaceId outFaceId = static_cast<FaceId>(interest.getNextHopFaceId());
  shared_ptr<Face> outFace = this->getFace(outFaceId);
  if (!static_cast<bool>(outFace)) {
    // If outFace doesn't exist, it's better to reject the Interest
    // than to use BestRouteStrategy.
    NFD_LOG_WARN("Interest " << interest.getName() <<
                 " NextHopFaceId=" << outFaceId << " non-existent face");
    this->rejectPendingInterest(pitEntry);
    return;
  }

  this->sendInterest(pitEntry, outFace);
}

} // namespace fw
} // namespace nfd
