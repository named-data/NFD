/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "access-strategy.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("AccessStrategy");

const Name AccessStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/access/%FD%01");
NFD_REGISTER_STRATEGY(AccessStrategy);

AccessStrategy::AccessStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
  , m_removeFaceInfoConn(this->beforeRemoveFace.connect(
                         bind(&AccessStrategy::removeFaceInfo, this, _1)))
{
}

AccessStrategy::~AccessStrategy()
{
}

void
AccessStrategy::afterReceiveInterest(const Face& inFace,
                                     const Interest& interest,
                                     shared_ptr<fib::Entry> fibEntry,
                                     shared_ptr<pit::Entry> pitEntry)
{
  RetxSuppression::Result suppressResult = m_retxSuppression.decide(inFace, interest, *pitEntry);
  switch (suppressResult) {
  case RetxSuppression::NEW:
    this->afterReceiveNewInterest(inFace, interest, fibEntry, pitEntry);
    break;
  case RetxSuppression::FORWARD:
    this->afterReceiveRetxInterest(inFace, interest, fibEntry, pitEntry);
    break;
  case RetxSuppression::SUPPRESS:
    NFD_LOG_DEBUG(interest << " interestFrom " << inFace.getId() << " retx-suppress");
    break;
  default:
    BOOST_ASSERT(false);
    break;
  }
}

void
AccessStrategy::afterReceiveNewInterest(const Face& inFace,
                                        const Interest& interest,
                                        shared_ptr<fib::Entry> fibEntry,
                                        shared_ptr<pit::Entry> pitEntry)
{
  Name miName;
  shared_ptr<MtInfo> mi;
  std::tie(miName, mi) = this->findPrefixMeasurements(*pitEntry);

  // has measurements for Interest Name?
  if (mi != nullptr) {
    NFD_LOG_DEBUG(interest << " interestFrom " << inFace.getId() <<
                  " new-interest mi=" << miName);

    // send to last working nexthop
    bool isSentToLastNexthop = this->sendToLastNexthop(inFace, pitEntry, *mi, fibEntry);

    if (isSentToLastNexthop) {
      return;
    }
  }
  else {
    NFD_LOG_DEBUG(interest << " interestFrom " << inFace.getId() <<
                  " new-interest no-mi");
  }

  // no measurements, or last working nexthop unavailable

  // multicast to all nexthops except incoming face
  this->multicast(pitEntry, fibEntry, {inFace.getId()});
}

void
AccessStrategy::afterReceiveRetxInterest(const Face& inFace,
                                         const Interest& interest,
                                         shared_ptr<fib::Entry> fibEntry,
                                         shared_ptr<pit::Entry> pitEntry)
{
  NFD_LOG_DEBUG(interest << " interestFrom " << inFace.getId() << " retx-forward");
  this->multicast(pitEntry, fibEntry, {inFace.getId()});
}

bool
AccessStrategy::sendToLastNexthop(const Face& inFace, shared_ptr<pit::Entry> pitEntry, MtInfo& mi,
                                  shared_ptr<fib::Entry> fibEntry)
{
  if (mi.lastNexthop == INVALID_FACEID) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " no-last-nexthop");
    return false;
  }

  if (mi.lastNexthop == inFace.getId()) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-is-downstream");
    return false;
  }

  shared_ptr<Face> face = this->getFace(mi.lastNexthop);
  if (face == nullptr || !fibEntry->hasNextHop(face)) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-gone");
    return false;
  }

  if (pitEntry->violatesScope(*face)) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-violates-scope");
    return false;
  }

  RttEstimator::Duration rto = mi.rtt.computeRto();
  NFD_LOG_DEBUG(pitEntry->getInterest() << " interestTo " << mi.lastNexthop <<
                " last-nexthop rto=" << time::duration_cast<time::microseconds>(rto).count());

  this->sendInterest(pitEntry, face);

  // schedule RTO timeout
  shared_ptr<PitInfo> pi = pitEntry->getOrCreateStrategyInfo<PitInfo>();
  pi->rtoTimer = scheduler::schedule(rto,
      bind(&AccessStrategy::afterRtoTimeout, this, weak_ptr<pit::Entry>(pitEntry),
           weak_ptr<fib::Entry>(fibEntry), inFace.getId(), mi.lastNexthop));

  return true;
}

void
AccessStrategy::afterRtoTimeout(weak_ptr<pit::Entry> pitWeak, weak_ptr<fib::Entry> fibWeak,
                                FaceId inFace, FaceId firstOutFace)
{
  shared_ptr<pit::Entry> pitEntry = pitWeak.lock();
  BOOST_ASSERT(pitEntry != nullptr);
  // pitEntry can't become nullptr, because RTO timer should be cancelled upon pitEntry destruction

  shared_ptr<fib::Entry> fibEntry = fibWeak.lock();
  if (fibEntry == nullptr) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " timeoutFrom " << firstOutFace << " fib-gone");
    return;
  }

  NFD_LOG_DEBUG(pitEntry->getInterest() << " timeoutFrom " << firstOutFace <<
                " multicast-except " << inFace << ',' << firstOutFace);
  this->multicast(pitEntry, fibEntry, {inFace, firstOutFace});
}

void
AccessStrategy::multicast(shared_ptr<pit::Entry> pitEntry, shared_ptr<fib::Entry> fibEntry,
                          std::unordered_set<FaceId> exceptFaces)
{
  for (const fib::NextHop& nexthop : fibEntry->getNextHops()) {
    shared_ptr<Face> face = nexthop.getFace();
    if (exceptFaces.count(face->getId()) > 0) {
      continue;
    }
    NFD_LOG_DEBUG(pitEntry->getInterest() << " interestTo " << face->getId() <<
                  " multicast");
    this->sendInterest(pitEntry, face);
  }
}

void
AccessStrategy::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,
                                      const Face& inFace, const Data& data)
{
  shared_ptr<PitInfo> pi = pitEntry->getStrategyInfo<PitInfo>();
  if (pi != nullptr) {
    pi->rtoTimer.cancel();
  }

  if (pitEntry->getInRecords().empty()) { // already satisfied by another upstream
    NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << inFace.getId() <<
                  " not-fastest");
    return;
  }

  pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(inFace);
  if (outRecord == pitEntry->getOutRecords().end()) { // no OutRecord
    NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << inFace.getId() <<
                  " no-out-record");
    return;
  }

  time::steady_clock::Duration rtt = time::steady_clock::now() - outRecord->getLastRenewed();
  NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << inFace.getId() <<
                " rtt=" << time::duration_cast<time::microseconds>(rtt).count());
  this->updateMeasurements(inFace, data, time::duration_cast<RttEstimator::Duration>(rtt));
}

void
AccessStrategy::updateMeasurements(const Face& inFace, const Data& data,
                                   const RttEstimator::Duration& rtt)
{
  FaceInfo& fi = m_fit[inFace.getId()];
  fi.rtt.addMeasurement(rtt);

  shared_ptr<MtInfo> mi = this->addPrefixMeasurements(data);
  if (mi->lastNexthop != inFace.getId()) {
    mi->lastNexthop = inFace.getId();
    mi->rtt = fi.rtt;
  }
  else {
    mi->rtt.addMeasurement(rtt);
  }
}

AccessStrategy::MtInfo::MtInfo()
  : lastNexthop(INVALID_FACEID)
  , rtt(1, time::milliseconds(1), 0.1)
{
}

std::tuple<Name, shared_ptr<AccessStrategy::MtInfo>>
AccessStrategy::findPrefixMeasurements(const pit::Entry& pitEntry)
{
  shared_ptr<measurements::Entry> me = this->getMeasurements().findLongestPrefixMatch(pitEntry);
  if (me == nullptr) {
    return std::forward_as_tuple(Name(), nullptr);
  }

  shared_ptr<MtInfo> mi = me->getStrategyInfo<MtInfo>();
  BOOST_ASSERT(mi != nullptr);
  // XXX after runtime strategy change, it's possible that me exists but mi doesn't exist;
  // this case needs another longest prefix match until mi is found
  return std::forward_as_tuple(me->getName(), mi);
}

shared_ptr<AccessStrategy::MtInfo>
AccessStrategy::addPrefixMeasurements(const Data& data)
{
  shared_ptr<measurements::Entry> me;
  if (data.getName().size() >= 1) {
    me = this->getMeasurements().get(data.getName().getPrefix(-1));
  }
  if (me == nullptr) { // parent of Data Name is not in this strategy, or Data Name is empty
    me = this->getMeasurements().get(data.getName());
    // Data Name must be in this strategy
    BOOST_ASSERT(me != nullptr);
  }

  static const time::nanoseconds ME_LIFETIME = time::seconds(8);
  this->getMeasurements().extendLifetime(*me, ME_LIFETIME);

  return me->getOrCreateStrategyInfo<MtInfo>();
}

AccessStrategy::FaceInfo::FaceInfo()
  : rtt(1, time::milliseconds(1), 0.1)
{
}

void
AccessStrategy::removeFaceInfo(shared_ptr<Face> face)
{
  m_fit.erase(face->getId());
}

} // namespace fw
} // namespace nfd
