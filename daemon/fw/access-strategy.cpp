/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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
#include "algorithm.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("AccessStrategy");
NFD_REGISTER_STRATEGY(AccessStrategy);

AccessStrategy::AccessStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , m_removeFaceInfoConn(this->beforeRemoveFace.connect(
                         bind(&AccessStrategy::removeFaceInfo, this, _1)))
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    BOOST_THROW_EXCEPTION(std::invalid_argument("AccessStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
      "AccessStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
AccessStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/access/%FD%01");
  return strategyName;
}

void
AccessStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                     const shared_ptr<pit::Entry>& pitEntry)
{
  RetxSuppressionResult suppressResult = m_retxSuppression.decidePerPitEntry(*pitEntry);
  switch (suppressResult) {
  case RetxSuppressionResult::NEW:
    this->afterReceiveNewInterest(inFace, interest, pitEntry);
    break;
  case RetxSuppressionResult::FORWARD:
    this->afterReceiveRetxInterest(inFace, interest, pitEntry);
    break;
  case RetxSuppressionResult::SUPPRESS:
    NFD_LOG_DEBUG(interest << " interestFrom " << inFace.getId() << " retx-suppress");
    break;
  default:
    BOOST_ASSERT(false);
    break;
  }
}

void
AccessStrategy::afterReceiveNewInterest(const Face& inFace, const Interest& interest,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  Name miName;
  MtInfo* mi = nullptr;
  std::tie(miName, mi) = this->findPrefixMeasurements(*pitEntry);

  // has measurements for Interest Name?
  if (mi != nullptr) {
    NFD_LOG_DEBUG(interest << " interestFrom " << inFace.getId() <<
                  " new-interest mi=" << miName);

    // send to last working nexthop
    bool isSentToLastNexthop = this->sendToLastNexthop(inFace, interest, pitEntry, *mi, fibEntry);

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
  int nMulticastSent = this->multicast(inFace, interest, pitEntry, fibEntry);

  if (nMulticastSent < 1) {
    this->rejectPendingInterest(pitEntry);
  }
}

void
AccessStrategy::afterReceiveRetxInterest(const Face& inFace, const Interest& interest,
                                         const shared_ptr<pit::Entry>& pitEntry)
{
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  NFD_LOG_DEBUG(interest << " interestFrom " << inFace.getId() << " retx-forward");
  this->multicast(inFace, interest, pitEntry, fibEntry);
}

bool
AccessStrategy::sendToLastNexthop(const Face& inFace, const Interest& interest,
                                  const shared_ptr<pit::Entry>& pitEntry, MtInfo& mi,
                                  const fib::Entry& fibEntry)
{
  if (mi.lastNexthop == face::INVALID_FACEID) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " no-last-nexthop");
    return false;
  }

  if (mi.lastNexthop == inFace.getId()) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-is-downstream");
    return false;
  }

  Face* outFace = this->getFace(mi.lastNexthop);
  if (outFace == nullptr || !fibEntry.hasNextHop(*outFace)) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-gone");
    return false;
  }

  if (wouldViolateScope(inFace, interest, *outFace)) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-violates-scope");
    return false;
  }

  RttEstimator::Duration rto = mi.rtt.computeRto();
  NFD_LOG_DEBUG(pitEntry->getInterest() << " interestTo " << mi.lastNexthop <<
                " last-nexthop rto=" << time::duration_cast<time::microseconds>(rto).count());

  this->sendInterest(pitEntry, *outFace, interest);

  // schedule RTO timeout
  PitInfo* pi = pitEntry->insertStrategyInfo<PitInfo>().first;
  pi->rtoTimer = scheduler::schedule(rto,
      bind(&AccessStrategy::afterRtoTimeout, this, weak_ptr<pit::Entry>(pitEntry),
           inFace.getId(), mi.lastNexthop));

  return true;
}

void
AccessStrategy::afterRtoTimeout(weak_ptr<pit::Entry> pitWeak, FaceId inFaceId, FaceId firstOutFaceId)
{
  shared_ptr<pit::Entry> pitEntry = pitWeak.lock();
  BOOST_ASSERT(pitEntry != nullptr);
  // if pitEntry is gone, RTO timer should have been cancelled

  Face* inFace = this->getFace(inFaceId);
  if (inFace == nullptr) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " timeoutFrom " << firstOutFaceId <<
                  " inFace-gone " << inFaceId);
    return;
  }

  pit::InRecordCollection::iterator inRecord = pitEntry->getInRecord(*inFace);
  BOOST_ASSERT(inRecord != pitEntry->in_end());
  // in-record is erased only if Interest is satisfied, and RTO timer should have been cancelled
  // note: if this strategy is extended to send Nacks, that would also erase in-record,
  //       and RTO timer should be cancelled in that case as well

  const Interest& interest = inRecord->getInterest();

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);

  NFD_LOG_DEBUG(pitEntry->getInterest() << " timeoutFrom " << firstOutFaceId <<
                " multicast-except " << firstOutFaceId);
  this->multicast(*inFace, interest, pitEntry, fibEntry, firstOutFaceId);
}

int
AccessStrategy::multicast(const Face& inFace, const Interest& interest,
                          const shared_ptr<pit::Entry>& pitEntry, const fib::Entry& fibEntry,
                          FaceId exceptFace)
{
  int nSent = 0;
  for (const fib::NextHop& nexthop : fibEntry.getNextHops()) {
    Face& outFace = nexthop.getFace();
    if (&outFace == &inFace || outFace.getId() == exceptFace ||
        wouldViolateScope(inFace, interest, outFace)) {
      continue;
    }
    NFD_LOG_DEBUG(pitEntry->getInterest() << " interestTo " << outFace.getId() <<
                  " multicast");
    this->sendInterest(pitEntry, outFace, interest);
    ++nSent;
  }
  return nSent;
}

void
AccessStrategy::beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                                      const Face& inFace, const Data& data)
{
  PitInfo* pi = pitEntry->getStrategyInfo<PitInfo>();
  if (pi != nullptr) {
    pi->rtoTimer.cancel();
  }

  if (!pitEntry->hasInRecords()) { // already satisfied by another upstream
    NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << inFace.getId() <<
                  " not-fastest");
    return;
  }

  pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(inFace);
  if (outRecord == pitEntry->out_end()) { // no out-record
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
  ///\todo move FaceInfoTable out of AccessStrategy instance, to Measurements or somewhere else
  FaceInfo& fi = m_fit[inFace.getId()];
  fi.rtt.addMeasurement(rtt);

  MtInfo* mi = this->addPrefixMeasurements(data);
  if (mi->lastNexthop != inFace.getId()) {
    mi->lastNexthop = inFace.getId();
    mi->rtt = fi.rtt;
  }
  else {
    mi->rtt.addMeasurement(rtt);
  }
}

AccessStrategy::MtInfo::MtInfo()
  : lastNexthop(face::INVALID_FACEID)
  , rtt(1, time::milliseconds(1), 0.1)
{
}

std::tuple<Name, AccessStrategy::MtInfo*>
AccessStrategy::findPrefixMeasurements(const pit::Entry& pitEntry)
{
  measurements::Entry* me = this->getMeasurements().findLongestPrefixMatch(pitEntry);
  if (me == nullptr) {
    return std::make_tuple(Name(), nullptr);
  }

  MtInfo* mi = me->getStrategyInfo<MtInfo>();
  BOOST_ASSERT(mi != nullptr);
  // XXX after runtime strategy change, it's possible that me exists but mi doesn't exist;
  // this case needs another longest prefix match until mi is found
  return std::make_tuple(me->getName(), mi);
}

AccessStrategy::MtInfo*
AccessStrategy::addPrefixMeasurements(const Data& data)
{
  measurements::Entry* me = nullptr;
  if (!data.getName().empty()) {
    me = this->getMeasurements().get(data.getName().getPrefix(-1));
  }
  if (me == nullptr) { // parent of Data Name is not in this strategy, or Data Name is empty
    me = this->getMeasurements().get(data.getName());
    // Data Name must be in this strategy
    BOOST_ASSERT(me != nullptr);
  }

  static const time::nanoseconds ME_LIFETIME = time::seconds(8);
  this->getMeasurements().extendLifetime(*me, ME_LIFETIME);

  return me->insertStrategyInfo<MtInfo>().first;
}

AccessStrategy::FaceInfo::FaceInfo()
  : rtt(1, time::milliseconds(1), 0.1)
{
}

void
AccessStrategy::removeFaceInfo(const Face& face)
{
  m_fit.erase(face.getId());
}

} // namespace fw
} // namespace nfd
