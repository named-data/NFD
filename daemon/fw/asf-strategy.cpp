/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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
#include "algorithm.hpp"

#include "core/logger.hpp"

namespace nfd {
namespace fw {
namespace asf {

NFD_LOG_INIT("AsfStrategy");
NFD_REGISTER_STRATEGY(AsfStrategy);

const time::milliseconds AsfStrategy::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds AsfStrategy::RETX_SUPPRESSION_MAX(250);

AsfStrategy::AsfStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , m_measurements(getMeasurements())
  , m_probing(m_measurements)
  , m_maxSilentTimeouts(0)
  , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                      RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                      RETX_SUPPRESSION_MAX)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    processParams(parsed.parameters);
  }

  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
      "AsfStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));

  NFD_LOG_DEBUG("Probing interval=" << m_probing.getProbingInterval()
                << ", Num silent timeouts=" << m_maxSilentTimeouts);
}

const Name&
AsfStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/asf/%FD%03");
  return strategyName;
}

void
AsfStrategy::processParams(const PartialName& parsed)
{
  for (const auto& component : parsed) {
    std::string parsedStr(reinterpret_cast<const char*>(component.value()), component.value_size());
    auto n = parsedStr.find("~");
    if (n == std::string::npos) {
      BOOST_THROW_EXCEPTION(std::invalid_argument("Format is <parameter>~<value>"));
    }

    auto f = parsedStr.substr(0, n);
    auto s = parsedStr.substr(n + 1);
    if (f == "probing-interval") {
      m_probing.setProbingInterval(getParamValue(f, s));
    }
    else if (f == "n-silent-timeouts") {
      m_maxSilentTimeouts = getParamValue(f, s);
    }
    else {
      BOOST_THROW_EXCEPTION(std::invalid_argument("Parameter should be probing-interval or n-silent-timeouts"));
    }
  }
}

uint64_t
AsfStrategy::getParamValue(const std::string& param, const std::string& value)
{
  try {
    if (!value.empty() && value[0] == '-')
      BOOST_THROW_EXCEPTION(boost::bad_lexical_cast());

    return boost::lexical_cast<uint64_t>(value);
  }
  catch (const boost::bad_lexical_cast&) {
    BOOST_THROW_EXCEPTION(std::invalid_argument("Value of " + param + " must be a non-negative integer"));
  }
}

void
AsfStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                  const shared_ptr<pit::Entry>& pitEntry)
{
  // Should the Interest be suppressed?
  RetxSuppressionResult suppressResult = m_retxSuppression.decidePerPitEntry(*pitEntry);

  switch (suppressResult) {
  case RetxSuppressionResult::NEW:
  case RetxSuppressionResult::FORWARD:
    break;
  case RetxSuppressionResult::SUPPRESS:
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

  NFD_LOG_TRACE("Forwarding interest to face: " << faceToUse->getId());

  forwardInterest(interest, fibEntry, pitEntry, *faceToUse);

  // If necessary, send probe
  if (m_probing.isProbingNeeded(fibEntry, interest)) {
    Face* faceToProbe = m_probing.getFaceToProbe(inFace, interest, fibEntry, *faceToUse);

    if (faceToProbe != nullptr) {
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
  FaceInfo* faceInfo = namespaceInfo->get(inFace.getId());
  if (faceInfo == nullptr) {
    return;
  }
  faceInfo->recordRtt(pitEntry, inFace);

  // Extend lifetime for measurements associated with Face
  namespaceInfo->extendFaceInfoLifetime(*faceInfo, inFace.getId());

  if (faceInfo->isTimeoutScheduled()) {
    faceInfo->cancelTimeoutEvent(data.getName());
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
  if (wantNewNonce) {
    //Send probe: interest with new Nonce
    Interest probeInterest(interest);
    probeInterest.refreshNonce();
    NFD_LOG_TRACE("Sending probe for " << probeInterest << probeInterest.getNonce()
                                       << " to FaceId: " << outFace.getId());
    this->sendInterest(pitEntry, outFace, probeInterest);
  }
  else {
    this->sendInterest(pitEntry, outFace, interest);
  }

  FaceInfo& faceInfo = m_measurements.getOrCreateFaceInfo(fibEntry, interest, outFace.getId());

  // Refresh measurements since Face is being used for forwarding
  NamespaceInfo& namespaceInfo = m_measurements.getOrCreateNamespaceInfo(fibEntry, interest);
  namespaceInfo.extendFaceInfoLifetime(faceInfo, outFace.getId());

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

    if ((hopFace.getId() == inFace.getId() && hopFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
         wouldViolateScope(inFace, interest, hopFace)) {
      continue;
    }

    FaceInfo* info = m_measurements.getFaceInfo(fibEntry, interest, hopFace.getId());

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
AsfStrategy::onTimeout(const Name& interestName, const face::FaceId faceId)
{
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

  faceInfo.setNSilentTimeouts(faceInfo.getNSilentTimeouts() + 1);

  if (faceInfo.getNSilentTimeouts() <= m_maxSilentTimeouts) {
    NFD_LOG_TRACE("FaceId " << faceId << " for " << interestName << " has timed-out "
                  << faceInfo.getNSilentTimeouts() << " time(s), ignoring");
    // Extend lifetime for measurements associated with Face
    namespaceInfo->extendFaceInfoLifetime(faceInfo, faceId);

    if (faceInfo.isTimeoutScheduled()) {
      faceInfo.cancelTimeoutEvent(interestName);
    }
  }
  else {
    NFD_LOG_TRACE("FaceId " << faceId << " for " << interestName << " has timed-out");
    faceInfo.recordTimeout(interestName);
  }
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
