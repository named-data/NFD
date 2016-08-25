/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "asf-strategy.hpp"

#include "core/logger.hpp"

namespace nfd {
namespace fw {
namespace asf {

NFD_LOG_INIT("AsfStrategy");

const Name AsfStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/asf/%FD%01");
const time::milliseconds AsfStrategy::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds AsfStrategy::RETX_SUPPRESSION_MAX(250);

NFD_REGISTER_STRATEGY(AsfStrategy);

AsfStrategy::AsfStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
  , m_measurements(getMeasurements())
  , m_probing(m_measurements)
  , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                      RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                      RETX_SUPPRESSION_MAX)
{
}

void
AsfStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                  const shared_ptr<pit::Entry>& pitEntry)
{
  // Should the Interest be suppressed?
  RetxSuppression::Result suppressResult = m_retxSuppression.decide(inFace, interest, *pitEntry);

  switch (suppressResult) {
  case RetxSuppression::NEW:
  case RetxSuppression::FORWARD:
    break;
  case RetxSuppression::SUPPRESS:
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " suppressed");
    return;
  }

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  if (nexthops.size() == 0) {
    sendNoRouteNack(inFace, interest, pitEntry);
    this->rejectPendingInterest(pitEntry);
    return;
  }

  Face* faceToUse = getBestFaceForForwarding(fibEntry, interest, inFace);

  if (faceToUse == nullptr) {
    sendNoRouteNack(inFace, interest, pitEntry);
    this->rejectPendingInterest(pitEntry);
    return;
  }

  forwardInterest(interest, fibEntry, pitEntry, *faceToUse);

  // If necessary, send probe
  if (m_probing.isProbingNeeded(fibEntry, interest)) {
    Face* faceToProbe = m_probing.getFaceToProbe(inFace, interest, fibEntry, *faceToUse);

    if (faceToProbe != nullptr) {
      NFD_LOG_TRACE("Sending probe for " << fibEntry.getPrefix()
                                         << " to FaceId: " << faceToProbe->getId());

      bool wantNewNonce = true;
      forwardInterest(interest, fibEntry, pitEntry, *faceToProbe, wantNewNonce);
      m_probing.afterForwardingProbe(fibEntry, interest);
    }
  }
}

void
AsfStrategy::beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                                   const Face& inFace, const Data& data)
{
  NamespaceInfo* namespaceInfo = m_measurements.getNamespaceInfo(pitEntry->getName());

  if (namespaceInfo == nullptr) {
    NFD_LOG_TRACE("Could not find measurements entry for " << pitEntry->getName());
    return;
  }

  // Record the RTT between the Interest out to Data in
  FaceInfo& faceInfo = namespaceInfo->get(inFace.getId());
  faceInfo.recordRtt(pitEntry, inFace);

  // Extend lifetime for measurements associated with Face
  namespaceInfo->extendFaceInfoLifetime(faceInfo, inFace);

  if (faceInfo.isTimeoutScheduled()) {
    faceInfo.cancelTimeoutEvent(data.getName());
  }
}

void
AsfStrategy::afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                              const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("Nack for " << nack.getInterest() << " from=" << inFace.getId() << ": " << nack.getReason());
  onTimeout(pitEntry->getName(), inFace.getId());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void
AsfStrategy::forwardInterest(const Interest& interest,
                             const fib::Entry& fibEntry,
                             const shared_ptr<pit::Entry>& pitEntry,
                             Face& outFace,
                             bool wantNewNonce)
{
  this->sendInterest(pitEntry, outFace, wantNewNonce);

  FaceInfo& faceInfo = m_measurements.getOrCreateFaceInfo(fibEntry, interest, outFace);

  // Refresh measurements since Face is being used for forwarding
  NamespaceInfo& namespaceInfo = m_measurements.getOrCreateNamespaceInfo(fibEntry, interest);
  namespaceInfo.extendFaceInfoLifetime(faceInfo, outFace);

  if (!faceInfo.isTimeoutScheduled()) {
    // Estimate and schedule timeout
    RttEstimator::Duration timeout = faceInfo.computeRto();

    NFD_LOG_TRACE("Scheduling timeout for " << fibEntry.getPrefix()
                                            << " FaceId: " << outFace.getId()
                                            << " in " << time::duration_cast<time::milliseconds>(timeout) << " ms");

    scheduler::EventId id = scheduler::schedule(timeout,
        bind(&AsfStrategy::onTimeout, this, interest.getName(), outFace.getId()));

    faceInfo.setTimeoutEvent(id, interest.getName());
  }
}

struct FaceStats
{
  Face* face;
  RttStats::Rtt rtt;
  RttStats::Rtt srtt;
  uint64_t cost;
};

double
getValueForSorting(const FaceStats& stats)
{
  // These values allow faces with no measurements to be ranked better than timeouts
  // srtt < RTT_NO_MEASUREMENT < RTT_TIMEOUT
  static const RttStats::Rtt SORTING_RTT_TIMEOUT = time::microseconds::max();
  static const RttStats::Rtt SORTING_RTT_NO_MEASUREMENT = SORTING_RTT_TIMEOUT / 2;

  if (stats.rtt == RttStats::RTT_TIMEOUT) {
    return SORTING_RTT_TIMEOUT.count();
  }
  else if (stats.rtt == RttStats::RTT_NO_MEASUREMENT) {
    return SORTING_RTT_NO_MEASUREMENT.count();
  }
  else {
    return stats.srtt.count();
  }
}

Face*
AsfStrategy::getBestFaceForForwarding(const fib::Entry& fibEntry, const Interest& interest,
                                      const Face& inFace)
{
  NFD_LOG_TRACE("Looking for best face for " << fibEntry.getPrefix());

  typedef std::function<bool(const FaceStats&, const FaceStats&)> FaceStatsPredicate;
  typedef std::set<FaceStats, FaceStatsPredicate> FaceStatsSet;

  FaceStatsSet rankedFaces(
    [] (const FaceStats& lhs, const FaceStats& rhs) -> bool {
      // Sort by RTT and then by cost
      double lhsValue = getValueForSorting(lhs);
      double rhsValue = getValueForSorting(rhs);

      if (lhsValue < rhsValue) {
        return true;
      }
      else if (lhsValue == rhsValue) {
        return lhs.cost < rhs.cost;
      }
      else {
        return false;
      }
  });

  for (const fib::NextHop& hop : fibEntry.getNextHops()) {
    Face& hopFace = hop.getFace();

    if (hopFace.getId() == inFace.getId()) {
      continue;
    }

    FaceInfo* info = m_measurements.getFaceInfo(fibEntry, interest, hopFace);

    if (info == nullptr) {
      FaceStats stats = {&hopFace,
                         RttStats::RTT_NO_MEASUREMENT,
                         RttStats::RTT_NO_MEASUREMENT,
                         hop.getCost()};

      rankedFaces.insert(stats);
    }
    else {
      FaceStats stats = {&hopFace, info->getRtt(), info->getSrtt(), hop.getCost()};
      rankedFaces.insert(stats);
    }
  }

  FaceStatsSet::iterator it = rankedFaces.begin();

  if (it != rankedFaces.end()) {
    return it->face;
  }
  else {
    return nullptr;
  }
}

void
AsfStrategy::onTimeout(const Name& interestName, face::FaceId faceId)
{
  NFD_LOG_TRACE("FaceId: " << faceId << " for " << interestName << " has timed-out");

  NamespaceInfo* namespaceInfo = m_measurements.getNamespaceInfo(interestName);

  if (namespaceInfo == nullptr) {
    NFD_LOG_TRACE("FibEntry for " << interestName << " has been removed since timeout scheduling");
    return;
  }

  FaceInfoTable::iterator it = namespaceInfo->find(faceId);

  if (it == namespaceInfo->end()) {
    it = namespaceInfo->insert(faceId);
  }

  FaceInfo& faceInfo = it->second;
  faceInfo.recordTimeout(interestName);
}

void
AsfStrategy::sendNoRouteNack(const Face& inFace, const Interest& interest,
                             const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");

  lp::NackHeader nackHeader;
  nackHeader.setReason(lp::NackReason::NO_ROUTE);
  this->sendNack(pitEntry, inFace, nackHeader);
}

} // namespace asf
} // namespace fw
} // namespace nfd
