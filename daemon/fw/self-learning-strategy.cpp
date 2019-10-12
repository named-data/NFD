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

#include "self-learning-strategy.hpp"
#include "algorithm.hpp"

#include "common/global.hpp"
#include "common/logger.hpp"
#include "rib/service.hpp"

#include <ndn-cxx/lp/empty-value.hpp>
#include <ndn-cxx/lp/prefix-announcement-header.hpp>
#include <ndn-cxx/lp/tags.hpp>

#include <boost/range/adaptor/reversed.hpp>

namespace nfd {
namespace fw {

NFD_LOG_INIT(SelfLearningStrategy);
NFD_REGISTER_STRATEGY(SelfLearningStrategy);

const time::milliseconds SelfLearningStrategy::ROUTE_RENEW_LIFETIME(10_min);

SelfLearningStrategy::SelfLearningStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("SelfLearningStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument(
      "SelfLearningStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
SelfLearningStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/self-learning/%FD%01");
  return strategyName;
}

void
SelfLearningStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                           const shared_ptr<pit::Entry>& pitEntry)
{
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  bool isNonDiscovery = interest.getTag<lp::NonDiscoveryTag>() != nullptr;
  auto inRecordInfo = pitEntry->getInRecord(ingress.face)->insertStrategyInfo<InRecordInfo>().first;
  if (isNonDiscovery) { // "non-discovery" Interest
    inRecordInfo->isNonDiscoveryInterest = true;
    if (nexthops.empty()) { // return NACK if no matching FIB entry exists
      NFD_LOG_DEBUG("NACK non-discovery Interest=" << interest << " from=" << ingress << " noNextHop");
      lp::NackHeader nackHeader;
      nackHeader.setReason(lp::NackReason::NO_ROUTE);
      this->sendNack(pitEntry, ingress, nackHeader);
      this->rejectPendingInterest(pitEntry);
    }
    else { // multicast it if matching FIB entry exists
      multicastInterest(interest, ingress.face, pitEntry, nexthops);
    }
  }
  else { // "discovery" Interest
    inRecordInfo->isNonDiscoveryInterest = false;
    if (nexthops.empty()) { // broadcast it if no matching FIB entry exists
      broadcastInterest(interest, ingress.face, pitEntry);
    }
    else { // multicast it with "non-discovery" mark if matching FIB entry exists
      interest.setTag(make_shared<lp::NonDiscoveryTag>(lp::EmptyValue{}));
      multicastInterest(interest, ingress.face, pitEntry, nexthops);
    }
  }
}

void
SelfLearningStrategy::afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,
                                       const FaceEndpoint& ingress, const Data& data)
{
  auto outRecord = pitEntry->getOutRecord(ingress.face);
  if (outRecord == pitEntry->out_end()) {
    NFD_LOG_DEBUG("Data " << data.getName() << " from=" << ingress << " no out-record");
    return;
  }

  OutRecordInfo* outRecordInfo = outRecord->getStrategyInfo<OutRecordInfo>();
  if (outRecordInfo && outRecordInfo->isNonDiscoveryInterest) { // outgoing Interest was non-discovery
    if (!needPrefixAnn(pitEntry)) { // no need to attach a PA (common cases)
      sendDataToAll(pitEntry, ingress, data);
    }
    else { // needs a PA (to respond discovery Interest)
      asyncProcessData(pitEntry, ingress.face, data);
    }
  }
  else { // outgoing Interest was discovery
    auto paTag = data.getTag<lp::PrefixAnnouncementTag>();
    if (paTag != nullptr) {
      addRoute(pitEntry, ingress.face, data, *paTag->get().getPrefixAnn());
    }
    else { // Data contains no PrefixAnnouncement, upstreams do not support self-learning
    }
    sendDataToAll(pitEntry, ingress, data);
  }
}

void
SelfLearningStrategy::afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                                       const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("Nack for " << nack.getInterest() << " from=" << ingress
                << " reason=" << nack.getReason());
  if (nack.getReason() == lp::NackReason::NO_ROUTE) { // remove FIB entries
    BOOST_ASSERT(this->lookupFib(*pitEntry).hasNextHops());
    NFD_LOG_DEBUG("Send NACK to all downstreams");
    this->sendNacks(pitEntry, nack.getHeader());
    renewRoute(nack.getInterest().getName(), ingress.face.getId(), 0_ms);
  }
}

void
SelfLearningStrategy::broadcastInterest(const Interest& interest, const Face& inFace,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  for (auto& outFace : this->getFaceTable() | boost::adaptors::reversed) {
    if ((outFace.getId() == inFace.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
        wouldViolateScope(inFace, interest, outFace) || outFace.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
      continue;
    }
    this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
    pitEntry->getOutRecord(outFace)->insertStrategyInfo<OutRecordInfo>().first->isNonDiscoveryInterest = false;
    NFD_LOG_DEBUG("send discovery Interest=" << interest << " from="
                  << inFace.getId() << " to=" << outFace.getId());
  }
}

void
SelfLearningStrategy::multicastInterest(const Interest& interest, const Face& inFace,
                                        const shared_ptr<pit::Entry>& pitEntry,
                                        const fib::NextHopList& nexthops)
{
  for (const auto& nexthop : nexthops) {
    Face& outFace = nexthop.getFace();
    if ((outFace.getId() == inFace.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
        wouldViolateScope(inFace, interest, outFace)) {
      continue;
    }
    this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
    pitEntry->getOutRecord(outFace)->insertStrategyInfo<OutRecordInfo>().first->isNonDiscoveryInterest = true;
    NFD_LOG_DEBUG("send non-discovery Interest=" << interest << " from="
                  << inFace.getId() << " to=" << outFace.getId());
  }
}

void
SelfLearningStrategy::asyncProcessData(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace, const Data& data)
{
  // Given that this processing is asynchronous, the PIT entry's expiry timer is extended first
  // to ensure that the entry will not be removed before the whole processing is finished
  // (the PIT entry's expiry timer was set to 0 before dispatching)
  this->setExpiryTimer(pitEntry, 1_s);

  runOnRibIoService([pitEntryWeak = weak_ptr<pit::Entry>{pitEntry}, inFaceId = inFace.getId(), data, this] {
    rib::Service::get().getRibManager().slFindAnn(data.getName(),
      [pitEntryWeak, inFaceId, data, this] (optional<ndn::PrefixAnnouncement> paOpt) {
        if (paOpt) {
          runOnMainIoService([pitEntryWeak, inFaceId, data, pa = std::move(*paOpt), this] {
            auto pitEntry = pitEntryWeak.lock();
            auto inFace = this->getFace(inFaceId);
            if (pitEntry && inFace) {
              NFD_LOG_DEBUG("found PrefixAnnouncement=" << pa.getAnnouncedName());
              data.setTag(make_shared<lp::PrefixAnnouncementTag>(lp::PrefixAnnouncementHeader(pa)));
              this->sendDataToAll(pitEntry, FaceEndpoint(*inFace, 0), data);
              this->setExpiryTimer(pitEntry, 0_ms);
            }
            else {
              NFD_LOG_DEBUG("PIT entry or Face no longer exists");
            }
          });
        }
    });
  });
}

bool
SelfLearningStrategy::needPrefixAnn(const shared_ptr<pit::Entry>& pitEntry)
{
  bool hasDiscoveryInterest = false;
  bool directToConsumer = true;

  auto now = time::steady_clock::now();
  for (const auto& inRecord : pitEntry->getInRecords()) {
    if (inRecord.getExpiry() > now) {
      InRecordInfo* inRecordInfo = inRecord.getStrategyInfo<InRecordInfo>();
      if (inRecordInfo && !inRecordInfo->isNonDiscoveryInterest) {
        hasDiscoveryInterest = true;
      }
      if (inRecord.getFace().getScope() != ndn::nfd::FACE_SCOPE_LOCAL) {
        directToConsumer = false;
      }
    }
  }
  return hasDiscoveryInterest && !directToConsumer;
}

void
SelfLearningStrategy::addRoute(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace,
                               const Data& data, const ndn::PrefixAnnouncement& pa)
{
  runOnRibIoService([pitEntryWeak = weak_ptr<pit::Entry>{pitEntry}, inFaceId = inFace.getId(), data, pa] {
    rib::Service::get().getRibManager().slAnnounce(pa, inFaceId, ROUTE_RENEW_LIFETIME,
      [] (RibManager::SlAnnounceResult res) {
        NFD_LOG_DEBUG("Add route via PrefixAnnouncement with result=" << res);
      });
  });
}

void
SelfLearningStrategy::renewRoute(const Name& name, FaceId inFaceId, time::milliseconds maxLifetime)
{
  // renew route with PA or ignore PA (if route has no PA)
  runOnRibIoService([name, inFaceId, maxLifetime] {
    rib::Service::get().getRibManager().slRenew(name, inFaceId, maxLifetime,
      [] (RibManager::SlAnnounceResult res) {
        NFD_LOG_DEBUG("Renew route with result=" << res);
      });
  });
}

} // namespace fw
} // namespace nfd
