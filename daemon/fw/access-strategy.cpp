/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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
#include "common/global.hpp"
#include "common/logger.hpp"

namespace nfd::fw {

NFD_LOG_INIT(AccessStrategy);
NFD_REGISTER_STRATEGY(AccessStrategy);

AccessStrategy::AccessStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , m_rttEstimatorOpts(make_shared<RttEstimator::Options>()) // use the default options
  , m_removeFaceConn(beforeRemoveFace.connect([this] (const Face& face) { m_fit.erase(face.getId()); }))
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("AccessStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument("AccessStrategy does not support version " +
                                    std::to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
AccessStrategy::getStrategyName()
{
  static const auto strategyName = Name("/localhost/nfd/strategy/access").appendVersion(1);
  return strategyName;
}

void
AccessStrategy::afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                                     const shared_ptr<pit::Entry>& pitEntry)
{
  switch (auto res = m_retxSuppression.decidePerPitEntry(*pitEntry); res) {
  case RetxSuppressionResult::NEW:
    return afterReceiveNewInterest(interest, ingress, pitEntry);
  case RetxSuppressionResult::FORWARD:
    return afterReceiveRetxInterest(interest, ingress, pitEntry);
  case RetxSuppressionResult::SUPPRESS:
    NFD_LOG_INTEREST_FROM(interest, ingress, "suppressed");
    return;
  }
}

void
AccessStrategy::afterReceiveNewInterest(const Interest& interest, const FaceEndpoint& ingress,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  const auto& fibEntry = this->lookupFib(*pitEntry);
  auto [miName, mi] = this->findPrefixMeasurements(*pitEntry);

  // has measurements for Interest Name?
  if (mi != nullptr) {
    NFD_LOG_INTEREST_FROM(interest, ingress, "new mi=" << miName);

    // send to last working nexthop
    bool isSentToLastNexthop = this->sendToLastNexthop(interest, ingress, pitEntry, *mi, fibEntry);
    if (isSentToLastNexthop) {
      return;
    }
  }
  else {
    NFD_LOG_INTEREST_FROM(interest, ingress, "new no-mi");
  }

  // no measurements, or last working nexthop unavailable

  // multicast to all nexthops except incoming face
  size_t nMulticastSent = this->multicast(interest, ingress.face, pitEntry, fibEntry);

  if (nMulticastSent == 0) {
    this->rejectPendingInterest(pitEntry);
  }
}

void
AccessStrategy::afterReceiveRetxInterest(const Interest& interest, const FaceEndpoint& ingress,
                                         const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_INTEREST_FROM(interest, ingress, "retx");
  const auto& fibEntry = this->lookupFib(*pitEntry);
  this->multicast(interest, ingress.face, pitEntry, fibEntry);
}

bool
AccessStrategy::sendToLastNexthop(const Interest& interest, const FaceEndpoint& ingress,
                                  const shared_ptr<pit::Entry>& pitEntry, MtInfo& mi,
                                  const fib::Entry& fibEntry)
{
  if (mi.lastNexthop == face::INVALID_FACEID) {
    NFD_LOG_INTEREST_FROM(interest, ingress, "no-last-nexthop");
    return false;
  }

  if (mi.lastNexthop == ingress.face.getId()) {
    NFD_LOG_INTEREST_FROM(interest, ingress, "last-nexthop-is-downstream");
    return false;
  }

  Face* outFace = this->getFace(mi.lastNexthop);
  if (outFace == nullptr || !fibEntry.hasNextHop(*outFace)) {
    NFD_LOG_INTEREST_FROM(interest, ingress, "last-nexthop-gone");
    return false;
  }

  if (wouldViolateScope(ingress.face, interest, *outFace)) {
    NFD_LOG_INTEREST_FROM(interest, ingress, "last-nexthop-violates-scope");
    return false;
  }

  auto rto = mi.rtt.getEstimatedRto();
  NFD_LOG_INTEREST_FROM(interest, ingress, "to=" << mi.lastNexthop << " last-nexthop rto="
                        << time::duration_cast<time::microseconds>(rto).count() << "us");

  if (!this->sendInterest(interest, *outFace, pitEntry)) {
    return false;
  }

  // schedule RTO timeout
  PitInfo* pi = pitEntry->insertStrategyInfo<PitInfo>().first;
  pi->rtoTimer = getScheduler().schedule(rto,
    [this, pitWeak = weak_ptr<pit::Entry>(pitEntry), face = ingress.face.getId(), nh = mi.lastNexthop] {
      afterRtoTimeout(pitWeak, face, nh);
    });

  return true;
}

void
AccessStrategy::afterRtoTimeout(const weak_ptr<pit::Entry>& pitWeak,
                                FaceId inFaceId, FaceId firstOutFaceId)
{
  shared_ptr<pit::Entry> pitEntry = pitWeak.lock();
  // if PIT entry is gone, RTO timer should have been cancelled
  BOOST_ASSERT(pitEntry != nullptr);

  Face* inFace = this->getFace(inFaceId);
  if (inFace == nullptr) {
    NFD_LOG_DEBUG(pitEntry->getName() << " timeout from=" << firstOutFaceId
                  << " in-face-gone=" << inFaceId);
    return;
  }

  auto inRecord = pitEntry->findInRecord(*inFace);
  // in-record is erased only if Interest is satisfied, and RTO timer should have been cancelled
  // note: if this strategy is extended to send Nacks, that would also erase the in-record,
  //       and the RTO timer should be cancelled in that case as well
  BOOST_ASSERT(inRecord != pitEntry->in_end());

  const Interest& interest = inRecord->getInterest();
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);

  NFD_LOG_DEBUG(pitEntry->getName() << " timeout from=" << firstOutFaceId << " multicast");
  this->multicast(interest, *inFace, pitEntry, fibEntry, firstOutFaceId);
}

size_t
AccessStrategy::multicast(const Interest& interest, const Face& inFace,
                          const shared_ptr<pit::Entry>& pitEntry, const fib::Entry& fibEntry,
                          FaceId exceptFace)
{
  size_t nSent = 0;
  for (const auto& nexthop : fibEntry.getNextHops()) {
    Face& outFace = nexthop.getFace();
    if (&outFace == &inFace || outFace.getId() == exceptFace ||
        wouldViolateScope(inFace, interest, outFace)) {
      continue;
    }
    NFD_LOG_DEBUG(interest.getName() << " nonce=" << interest.getNonce()
                  << " multicast to=" << outFace.getId());
    if (this->sendInterest(interest, outFace, pitEntry)) {
      ++nSent;
    }
  }
  return nSent;
}

void
AccessStrategy::beforeSatisfyInterest(const Data& data, const FaceEndpoint& ingress,
                                      const shared_ptr<pit::Entry>& pitEntry)
{
  PitInfo* pi = pitEntry->getStrategyInfo<PitInfo>();
  if (pi != nullptr) {
    pi->rtoTimer.cancel();
  }

  if (!pitEntry->hasInRecords()) { // already satisfied by another upstream
    NFD_LOG_DATA_FROM(data, ingress, "not-fastest");
    return;
  }

  auto outRecord = pitEntry->findOutRecord(ingress.face);
  if (outRecord == pitEntry->out_end()) {
    NFD_LOG_DATA_FROM(data, ingress, "no-out-record");
    return;
  }

  auto rtt = time::steady_clock::now() - outRecord->getLastRenewed();
  NFD_LOG_DATA_FROM(data, ingress, "rtt=" << time::duration_cast<time::microseconds>(rtt).count() << "us");
  this->updateMeasurements(ingress.face, data, rtt);
}

void
AccessStrategy::updateMeasurements(const Face& inFace, const Data& data, time::nanoseconds rtt)
{
  FaceInfo& fi = m_fit.try_emplace(inFace.getId(), m_rttEstimatorOpts).first->second;
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

std::tuple<Name, AccessStrategy::MtInfo*>
AccessStrategy::findPrefixMeasurements(const pit::Entry& pitEntry)
{
  auto me = this->getMeasurements().findLongestPrefixMatch(pitEntry);
  if (me == nullptr) {
    return {Name{}, nullptr};
  }

  auto mi = me->getStrategyInfo<MtInfo>();
  // TODO: after a runtime strategy change, it's possible that a measurements::Entry exists but
  //       the corresponding MtInfo doesn't exist (mi == nullptr); this case needs another longest
  //       prefix match until an MtInfo is found.
  return {me->getName(), mi};
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

  this->getMeasurements().extendLifetime(*me, 8_s);
  return me->insertStrategyInfo<MtInfo>(m_rttEstimatorOpts).first;
}

} // namespace nfd::fw
