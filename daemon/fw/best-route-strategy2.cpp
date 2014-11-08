/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("BestRouteStrategy2");

const Name BestRouteStrategy2::STRATEGY_NAME("ndn:/localhost/nfd/strategy/best-route/%FD%02");
/// \todo don't use fixed interval; make it adaptive or use exponential back-off #1913
const time::milliseconds BestRouteStrategy2::MIN_RETRANSMISSION_INTERVAL(100);

BestRouteStrategy2::BestRouteStrategy2(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}

/** \brief determines whether a NextHop is eligible
 *  \param currentDownstream incoming FaceId of current Interest
 *  \param wantUnused if true, NextHop must not have unexpired OutRecord
 *  \param now time::steady_clock::now(), ignored if !wantUnused
 */
static inline bool
predicate_NextHop_eligible(const shared_ptr<pit::Entry>& pitEntry,
  const fib::NextHop& nexthop, FaceId currentDownstream,
  bool wantUnused = false,
  time::steady_clock::TimePoint now = time::steady_clock::TimePoint::min())
{
  shared_ptr<Face> upstream = nexthop.getFace();

  // upstream is current downstream
  if (upstream->getId() == currentDownstream)
    return false;

  // forwarding would violate scope
  if (pitEntry->violatesScope(*upstream))
    return false;

  if (wantUnused) {
    // NextHop must not have unexpired OutRecord
    pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(*upstream);
    if (outRecord != pitEntry->getOutRecords().end() &&
        outRecord->getExpiry() > now) {
      return false;
    }
  }

  return true;
}

static inline bool
compare_OutRecord_lastRenewed(const pit::OutRecord& a, const pit::OutRecord& b)
{
  return a.getLastRenewed() < b.getLastRenewed();
}

/** \brief pick an eligible NextHop with earliest OutRecord
 *  \note It is assumed that every nexthop has an OutRecord
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
    pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(*it->getFace());
    BOOST_ASSERT(outRecord != pitEntry->getOutRecords().end());
    if (outRecord->getLastRenewed() < earliestRenewed) {
      found = it;
      earliestRenewed = outRecord->getLastRenewed();
    }
  }
  return found;
}

void
BestRouteStrategy2::afterReceiveInterest(const Face& inFace,
                                         const Interest& interest,
                                         shared_ptr<fib::Entry> fibEntry,
                                         shared_ptr<pit::Entry> pitEntry)
{
  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  fib::NextHopList::const_iterator it = nexthops.end();

  bool isNewPitEntry = !pitEntry->hasUnexpiredOutRecords();
  if (isNewPitEntry) {
    // forward to nexthop with lowest cost except downstream
    it = std::find_if(nexthops.begin(), nexthops.end(),
      bind(&predicate_NextHop_eligible, pitEntry, _1, inFace.getId(),
           false, time::steady_clock::TimePoint::min()));

    if (it == nexthops.end()) {
      NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");
      this->rejectPendingInterest(pitEntry);
      return;
    }

    shared_ptr<Face> outFace = it->getFace();
    this->sendInterest(pitEntry, outFace);
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                           << " newPitEntry-to=" << outFace->getId());
    return;
  }

  // when was the last outgoing Interest?
  const pit::OutRecordCollection& outRecords = pitEntry->getOutRecords();
  pit::OutRecordCollection::const_iterator lastOutgoing = std::max_element(
    outRecords.begin(), outRecords.end(), &compare_OutRecord_lastRenewed);
  BOOST_ASSERT(lastOutgoing != outRecords.end()); // otherwise it's new PIT entry

  time::steady_clock::TimePoint now = time::steady_clock::now();
  time::steady_clock::Duration sinceLastOutgoing = now - lastOutgoing->getLastRenewed();
  bool shouldRetransmit = sinceLastOutgoing >= MIN_RETRANSMISSION_INTERVAL;
  if (!shouldRetransmit) {
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                           << " dontRetransmit sinceLastOutgoing=" << sinceLastOutgoing.count());
    return;
  }

  // find an unused upstream with lowest cost except downstream
  it = std::find_if(nexthops.begin(), nexthops.end(),
    bind(&predicate_NextHop_eligible, pitEntry, _1, inFace.getId(), true, now));
  if (it != nexthops.end()) {
    shared_ptr<Face> outFace = it->getFace();
    this->sendInterest(pitEntry, outFace);
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                           << " retransmit-unused-to=" << outFace->getId());
    return;
  }

  // find an eligible upstream that is used earliest
  it = findEligibleNextHopWithEarliestOutRecord(pitEntry, nexthops, inFace.getId());
  if (it == nexthops.end()) {
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " retransmitNoNextHop");
  }
  else {
    shared_ptr<Face> outFace = it->getFace();
    this->sendInterest(pitEntry, outFace);
    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                           << " retransmit-retry-to=" << outFace->getId());
  }
}

} // namespace fw
} // namespace nfd
