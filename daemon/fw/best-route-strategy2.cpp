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

#include "best-route-strategy2.hpp"
#include "pit-algorithm.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("BestRouteStrategy2");

const Name BestRouteStrategy2::STRATEGY_NAME("ndn:/localhost/nfd/strategy/best-route/%FD%04");
NFD_REGISTER_STRATEGY(BestRouteStrategy2);

const time::milliseconds BestRouteStrategy2::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds BestRouteStrategy2::RETX_SUPPRESSION_MAX(250);

BestRouteStrategy2::BestRouteStrategy2(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
  , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                      RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                      RETX_SUPPRESSION_MAX)
{
}

/** \brief determines whether a NextHop is eligible
 *  \param pitEntry PIT entry
 *  \param nexthop next hop
 *  \param currentDownstream incoming FaceId of current Interest
 *  \param wantUnused if true, NextHop must not have unexpired out-record
 *  \param now time::steady_clock::now(), ignored if !wantUnused
 */
static inline bool
predicate_NextHop_eligible(const shared_ptr<pit::Entry>& pitEntry,
  const fib::NextHop& nexthop, FaceId currentDownstream,
  bool wantUnused = false,
  time::steady_clock::TimePoint now = time::steady_clock::TimePoint::min())
{
  Face& upstream = nexthop.getFace();

  // upstream is current downstream
  if (upstream.getId() == currentDownstream)
    return false;

  // forwarding would violate scope
  if (violatesScope(*pitEntry, upstream))
    return false;

  if (wantUnused) {
    // NextHop must not have unexpired out-record
    pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(upstream);
    if (outRecord != pitEntry->out_end() && outRecord->getExpiry() > now) {
      return false;
    }
  }

  return true;
}

/** \brief pick an eligible NextHop with earliest out-record
 *  \note It is assumed that every nexthop has an out-record.
 */
static inline fib::NextHopList::const_iterator
findEligibleNextHopWithEarliestOutRecord(const shared_ptr<pit::Entry>& pitEntry,
                                         const fib::NextHopList& nexthops,
                                         FaceId currentDownstream)
{
  fib::NextHopList::const_iterator found = nexthops.end();
  time::steady_clock::TimePoint earliestRenewed = time::steady_clock::TimePoint::max();
  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    if (!predicate_NextHop_eligible(pitEntry, *it, currentDownstream))
      continue;
    pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(it->getFace());
    BOOST_ASSERT(outRecord != pitEntry->out_end());
    if (outRecord->getLastRenewed() < earliestRenewed) {
      found = it;
      earliestRenewed = outRecord->getLastRenewed();
    }
  }
  return found;
}

void
BestRouteStrategy2::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                         const shared_ptr<pit::Entry>& pitEntry)
{
  RetxSuppression::Result suppression = m_retxSuppression.decide(inFace, interest, *pitEntry);
  if (suppression == RetxSuppression::SUPPRESS) {
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                           << " suppressed");
    return;
  }

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();
  fib::NextHopList::const_iterator it = nexthops.end();

  if (suppression == RetxSuppression::NEW) {
    // forward to nexthop with lowest cost except downstream
    it = std::find_if(nexthops.begin(), nexthops.end(),
      bind(&predicate_NextHop_eligible, pitEntry, _1, inFace.getId(),
           false, time::steady_clock::TimePoint::min()));

    if (it == nexthops.end()) {
      NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");

      lp::NackHeader nackHeader;
      nackHeader.setReason(lp::NackReason::NO_ROUTE);
      this->sendNack(pitEntry, inFace, nackHeader);

      this->rejectPendingInterest(pitEntry);
      return;
    }

    Face& outFace = it->getFace();
    this->sendInterest(pitEntry, outFace);
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                           << " newPitEntry-to=" << outFace.getId());
    return;
  }

  // find an unused upstream with lowest cost except downstream
  it = std::find_if(nexthops.begin(), nexthops.end(),
                    bind(&predicate_NextHop_eligible, pitEntry, _1, inFace.getId(),
                         true, time::steady_clock::now()));
  if (it != nexthops.end()) {
    Face& outFace = it->getFace();
    this->sendInterest(pitEntry, outFace);
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                           << " retransmit-unused-to=" << outFace.getId());
    return;
  }

  // find an eligible upstream that is used earliest
  it = findEligibleNextHopWithEarliestOutRecord(pitEntry, nexthops, inFace.getId());
  if (it == nexthops.end()) {
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " retransmitNoNextHop");
  }
  else {
    Face& outFace = it->getFace();
    this->sendInterest(pitEntry, outFace);
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                           << " retransmit-retry-to=" << outFace.getId());
  }
}

/** \return less severe NackReason between x and y
 *
 *  lp::NackReason::NONE is treated as most severe
 */
inline lp::NackReason
compareLessSevere(lp::NackReason x, lp::NackReason y)
{
  if (x == lp::NackReason::NONE) {
    return y;
  }
  if (y == lp::NackReason::NONE) {
    return x;
  }
  return static_cast<lp::NackReason>(std::min(static_cast<int>(x), static_cast<int>(y)));
}

void
BestRouteStrategy2::afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                                     const shared_ptr<pit::Entry>& pitEntry)
{
  int nOutRecordsNotNacked = 0;
  Face* lastFaceNotNacked = nullptr;
  lp::NackReason leastSevereReason = lp::NackReason::NONE;
  for (const pit::OutRecord& outR : pitEntry->getOutRecords()) {
    const lp::NackHeader* inNack = outR.getIncomingNack();
    if (inNack == nullptr) {
      ++nOutRecordsNotNacked;
      lastFaceNotNacked = &outR.getFace();
      continue;
    }

    leastSevereReason = compareLessSevere(leastSevereReason, inNack->getReason());
  }

  lp::NackHeader outNack;
  outNack.setReason(leastSevereReason);

  if (nOutRecordsNotNacked == 1) {
    BOOST_ASSERT(lastFaceNotNacked != nullptr);
    pit::InRecordCollection::iterator inR = pitEntry->getInRecord(*lastFaceNotNacked);
    if (inR != pitEntry->in_end()) {
      // one out-record not Nacked, which is also a downstream
      NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                    " nack=" << nack.getReason() <<
                    " nack-to(bidirectional)=" << lastFaceNotNacked->getId() <<
                    " out-nack=" << outNack.getReason());
      this->sendNack(pitEntry, *lastFaceNotNacked, outNack);
      return;
    }
  }

  if (nOutRecordsNotNacked > 0) {
    NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                  " nack=" << nack.getReason() <<
                  " waiting=" << nOutRecordsNotNacked);
    // continue waiting
    return;
  }


  NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                " nack=" << nack.getReason() <<
                " nack-to=all out-nack=" << outNack.getReason());
  this->sendNacks(pitEntry, outNack);
}

} // namespace fw
} // namespace nfd
