/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

namespace nfd {
namespace fw {

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
    NDN_THROW(std::invalid_argument("AccessStrategy does not support version " + to_string(*parsed.version)));
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
AccessStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                     const shared_ptr<pit::Entry>& pitEntry)
{
  auto suppressResult = m_retxSuppression.decidePerPitEntry(*pitEntry);
  switch (suppressResult) {
  case RetxSuppressionResult::NEW:
    return afterReceiveNewInterest(ingress, interest, pitEntry);
  case RetxSuppressionResult::FORWARD:
    return afterReceiveRetxInterest(ingress, interest, pitEntry);
  case RetxSuppressionResult::SUPPRESS:
    NFD_LOG_DEBUG(interest << " interestFrom " << ingress << " retx-suppress");
    return;
  }
}

void
AccessStrategy::afterReceiveNewInterest(const FaceEndpoint& ingress, const Interest& interest,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  const auto& fibEntry = this->lookupFib(*pitEntry);
  Name miName;
  MtInfo* mi = nullptr;
  std::tie(miName, mi) = this->findPrefixMeasurements(*pitEntry);

  // has measurements for Interest Name?
  if (mi != nullptr) {
    NFD_LOG_DEBUG(interest << " interestFrom " << ingress << " new-interest mi=" << miName);

    // send to last working nexthop
    bool isSentToLastNexthop = this->sendToLastNexthop(ingress, interest, pitEntry, *mi, fibEntry);
    if (isSentToLastNexthop) {
      return;
    }
  }
  else {
    NFD_LOG_DEBUG(interest << " interestFrom " << ingress << " new-interest no-mi");
  }

  // no measurements, or last working nexthop unavailable

  // multicast to all nexthops except incoming face
  size_t nMulticastSent = this->multicast(ingress.face, interest, pitEntry, fibEntry);

  if (nMulticastSent == 0) {
    this->rejectPendingInterest(pitEntry);
  }
}

void
AccessStrategy::afterReceiveRetxInterest(const FaceEndpoint& ingress, const Interest& interest,
                                         const shared_ptr<pit::Entry>& pitEntry)
{
  const auto& fibEntry = this->lookupFib(*pitEntry);
  NFD_LOG_DEBUG(interest << " interestFrom " << ingress << " retx-forward");
  this->multicast(ingress.face, interest, pitEntry, fibEntry);
}

bool
AccessStrategy::sendToLastNexthop(const FaceEndpoint& ingress, const Interest& interest,
                                  const shared_ptr<pit::Entry>& pitEntry, MtInfo& mi,
                                  const fib::Entry& fibEntry)
{
  if (mi.lastNexthop == face::INVALID_FACEID) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " no-last-nexthop");
    return false;
  }

  if (mi.lastNexthop == ingress.face.getId()) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-is-downstream");
    return false;
  }

  Face* outFace = this->getFace(mi.lastNexthop);
  if (outFace == nullptr || !fibEntry.hasNextHop(*outFace)) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-gone");
    return false;
  }

  if (wouldViolateScope(ingress.face, interest, *outFace)) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " last-nexthop-violates-scope");
    return false;
  }

  auto rto = mi.rtt.getEstimatedRto();
  NFD_LOG_DEBUG(pitEntry->getInterest() << " interestTo " << mi.lastNexthop
                << " last-nexthop rto=" << time::duration_cast<time::microseconds>(rto).count());

  this->sendInterest(pitEntry, FaceEndpoint(*outFace, 0), interest);

  // schedule RTO timeout
  PitInfo* pi = pitEntry->insertStrategyInfo<PitInfo>().first;
  pi->rtoTimer = getScheduler().schedule(rto,
    [this, pitWeak = weak_ptr<pit::Entry>(pitEntry), face = ingress.face.getId(),
     endpoint = ingress.endpoint, lastNexthop = mi.lastNexthop] {
      afterRtoTimeout(pitWeak, face, endpoint, lastNexthop);
    });

  return true;
}

void
AccessStrategy::afterRtoTimeout(const weak_ptr<pit::Entry>& pitWeak,
                                FaceId inFaceId, EndpointId inEndpointId, FaceId firstOutFaceId)
{
  shared_ptr<pit::Entry> pitEntry = pitWeak.lock();
  // if PIT entry is gone, RTO timer should have been cancelled
  BOOST_ASSERT(pitEntry != nullptr);

  Face* inFace = this->getFace(inFaceId);
  if (inFace == nullptr) {
    NFD_LOG_DEBUG(pitEntry->getInterest() << " timeoutFrom " << firstOutFaceId
                  << " inFace-gone " << inFaceId);
    return;
  }

  auto inRecord = pitEntry->getInRecord(*inFace);
  // in-record is erased only if Interest is satisfied, and RTO timer should have been cancelled
  // note: if this strategy is extended to send Nacks, that would also erase the in-record,
  //       and the RTO timer should be cancelled in that case as well
  BOOST_ASSERT(inRecord != pitEntry->in_end());

  const Interest& interest = inRecord->getInterest();
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);

  NFD_LOG_DEBUG(pitEntry->getInterest() << " timeoutFrom " << firstOutFaceId
                << " multicast-except " << firstOutFaceId);
  this->multicast(*inFace, interest, pitEntry, fibEntry, firstOutFaceId);
}

size_t
AccessStrategy::multicast(const Face& inFace, const Interest& interest,
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
    NFD_LOG_DEBUG(pitEntry->getInterest() << " interestTo " << outFace.getId() << " multicast");
    this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
    ++nSent;
  }
  return nSent;
}

void
AccessStrategy::beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                                      const FaceEndpoint& ingress, const Data& data)
{
  PitInfo* pi = pitEntry->getStrategyInfo<PitInfo>();
  if (pi != nullptr) {
    pi->rtoTimer.cancel();
  }

  if (!pitEntry->hasInRecords()) { // already satisfied by another upstream
    NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << ingress << " not-fastest");
    return;
  }

  auto outRecord = pitEntry->getOutRecord(ingress.face);
  if (outRecord == pitEntry->out_end()) { // no out-record
    NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << ingress << " no-out-record");
    return;
  }

  auto rtt = time::steady_clock::now() - outRecord->getLastRenewed();
  NFD_LOG_DEBUG(pitEntry->getInterest() << " dataFrom " << ingress
                << " rtt=" << time::duration_cast<time::microseconds>(rtt).count());
  this->updateMeasurements(ingress.face, data, rtt);
}

void
AccessStrategy::updateMeasurements(const Face& inFace, const Data& data, time::nanoseconds rtt)
{
  auto ret = m_fit.emplace(std::piecewise_construct,
                           std::forward_as_tuple(inFace.getId()),
                           std::forward_as_tuple(m_rttEstimatorOpts));
  FaceInfo& fi = ret.first->second;
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

  this->getMeasurements().extendLifetime(*me, 8_s);
  return me->insertStrategyInfo<MtInfo>(m_rttEstimatorOpts).first;
}

} // namespace fw
} // namespace nfd
