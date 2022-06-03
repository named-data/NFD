/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#include "strategy.hpp"
#include "forwarder.hpp"
#include "common/logger.hpp"

#include <ndn-cxx/lp/pit-token.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <unordered_set>

namespace nfd::fw {

NFD_LOG_INIT(Strategy);

Strategy::Registry&
Strategy::getRegistry()
{
  static Registry registry;
  return registry;
}

Strategy::Registry::const_iterator
Strategy::find(const Name& instanceName)
{
  const Registry& registry = getRegistry();
  ParsedInstanceName parsed = parseInstanceName(instanceName);

  if (parsed.version) {
    // specified version: find exact or next higher version

    auto found = registry.lower_bound(parsed.strategyName);
    if (found != registry.end()) {
      if (parsed.strategyName.getPrefix(-1).isPrefixOf(found->first)) {
        NFD_LOG_TRACE("find " << instanceName << " versioned found=" << found->first);
        return found;
      }
    }

    NFD_LOG_TRACE("find " << instanceName << " versioned not-found");
    return registry.end();
  }

  // no version specified: find highest version

  if (!parsed.strategyName.empty()) { // Name().getSuccessor() would be invalid
    auto found = registry.lower_bound(parsed.strategyName.getSuccessor());
    if (found != registry.begin()) {
      --found;
      if (parsed.strategyName.isPrefixOf(found->first)) {
        NFD_LOG_TRACE("find " << instanceName << " unversioned found=" << found->first);
        return found;
      }
    }
  }

  NFD_LOG_TRACE("find " << instanceName << " unversioned not-found");
  return registry.end();
}

bool
Strategy::canCreate(const Name& instanceName)
{
  return Strategy::find(instanceName) != getRegistry().end();
}

unique_ptr<Strategy>
Strategy::create(const Name& instanceName, Forwarder& forwarder)
{
  auto found = Strategy::find(instanceName);
  if (found == getRegistry().end()) {
    NFD_LOG_DEBUG("create " << instanceName << " not-found");
    return nullptr;
  }

  unique_ptr<Strategy> instance = found->second(forwarder, instanceName);
  NFD_LOG_DEBUG("create " << instanceName << " found=" << found->first
                << " created=" << instance->getInstanceName());
  BOOST_ASSERT(!instance->getInstanceName().empty());
  return instance;
}

bool
Strategy::areSameType(const Name& instanceNameA, const Name& instanceNameB)
{
  return Strategy::find(instanceNameA) == Strategy::find(instanceNameB);
}

std::set<Name>
Strategy::listRegistered()
{
  std::set<Name> strategyNames;
  boost::copy(getRegistry() | boost::adaptors::map_keys,
              std::inserter(strategyNames, strategyNames.end()));
  return strategyNames;
}

Strategy::ParsedInstanceName
Strategy::parseInstanceName(const Name& input)
{
  for (ssize_t i = input.size() - 1; i > 0; --i) {
    if (input[i].isVersion()) {
      return {input.getPrefix(i + 1), input[i].toVersion(), input.getSubName(i + 1)};
    }
  }
  return {input, std::nullopt, PartialName()};
}

Name
Strategy::makeInstanceName(const Name& input, const Name& strategyName)
{
  BOOST_ASSERT(strategyName.at(-1).isVersion());

  bool hasVersion = std::any_of(input.rbegin(), input.rend(),
                                [] (const auto& comp) { return comp.isVersion(); });
  return hasVersion ? input : Name(input).append(strategyName.at(-1));
}

StrategyParameters
Strategy::parseParameters(const PartialName& params)
{
  StrategyParameters parsed;

  for (const auto& component : params) {
    auto sep = std::find(component.value_begin(), component.value_end(), '~');
    if (sep == component.value_end()) {
      NDN_THROW(std::invalid_argument("Strategy parameters format is (<parameter>~<value>)*"));
    }

    std::string p(component.value_begin(), sep);
    std::advance(sep, 1);
    std::string v(sep, component.value_end());
    if (p.empty() || v.empty()) {
      NDN_THROW(std::invalid_argument("Strategy parameter name and value cannot be empty"));
    }
    parsed[std::move(p)] = std::move(v);
  }

  return parsed;
}

Strategy::Strategy(Forwarder& forwarder)
  : afterAddFace(forwarder.m_faceTable.afterAdd)
  , beforeRemoveFace(forwarder.m_faceTable.beforeRemove)
  , m_forwarder(forwarder)
  , m_measurements(m_forwarder.getMeasurements(), m_forwarder.getStrategyChoice(), *this)
{
}

Strategy::~Strategy() = default;

void
Strategy::afterContentStoreHit(const Data& data, const FaceEndpoint& ingress,
                               const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("afterContentStoreHit pitEntry=" << pitEntry->getName()
                << " in=" << ingress << " data=" << data.getName());

  this->sendData(data, ingress.face, pitEntry);
}

void
Strategy::beforeSatisfyInterest(const Data& data, const FaceEndpoint& ingress,
                                const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("beforeSatisfyInterest pitEntry=" << pitEntry->getName()
                << " in=" << ingress << " data=" << data.getName());
}

void
Strategy::afterReceiveData(const Data& data, const FaceEndpoint& ingress,
                           const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("afterReceiveData pitEntry=" << pitEntry->getName()
                << " in=" << ingress << " data=" << data.getName());

  this->beforeSatisfyInterest(data, ingress, pitEntry);
  this->sendDataToAll(data, pitEntry, ingress.face);
}

void
Strategy::afterReceiveNack(const lp::Nack&, const FaceEndpoint& ingress,
                           const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("afterReceiveNack in=" << ingress << " pitEntry=" << pitEntry->getName());
}

void
Strategy::onDroppedInterest(const Interest& interest, Face& egress)
{
  NFD_LOG_DEBUG("onDroppedInterest out=" << egress.getId() << " name=" << interest.getName());
}

void
Strategy::afterNewNextHop(const fib::NextHop& nextHop, const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("afterNewNextHop pitEntry=" << pitEntry->getName()
                << " nexthop=" << nextHop.getFace().getId());
}

pit::OutRecord*
Strategy::sendInterest(const Interest& interest, Face& egress, const shared_ptr<pit::Entry>& pitEntry)
{
  if (interest.getTag<lp::PitToken>() != nullptr) {
    Interest interest2 = interest; // make a copy to preserve tag on original packet
    interest2.removeTag<lp::PitToken>();
    return m_forwarder.onOutgoingInterest(interest2, egress, pitEntry);
  }
  return m_forwarder.onOutgoingInterest(interest, egress, pitEntry);
}

bool
Strategy::sendData(const Data& data, Face& egress, const shared_ptr<pit::Entry>& pitEntry)
{
  BOOST_ASSERT(pitEntry->getInterest().matchesData(data));

  shared_ptr<lp::PitToken> pitToken;
  auto inRecord = pitEntry->getInRecord(egress);
  if (inRecord != pitEntry->in_end()) {
    pitToken = inRecord->getInterest().getTag<lp::PitToken>();
  }

  // delete the PIT entry's in-record based on egress,
  // since the Data is sent to the face from which the Interest was received
  pitEntry->deleteInRecord(egress);

  if (pitToken != nullptr) {
    Data data2 = data; // make a copy so each downstream can get a different PIT token
    data2.setTag(pitToken);
    return m_forwarder.onOutgoingData(data2, egress);
  }
  return m_forwarder.onOutgoingData(data, egress);
}

void
Strategy::sendDataToAll(const Data& data, const shared_ptr<pit::Entry>& pitEntry, const Face& inFace)
{
  std::set<Face*> pendingDownstreams;
  auto now = time::steady_clock::now();

  // remember pending downstreams
  for (const auto& inRecord : pitEntry->getInRecords()) {
    if (inRecord.getExpiry() > now) {
      if (inRecord.getFace().getId() == inFace.getId() &&
          inRecord.getFace().getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) {
        continue;
      }
      pendingDownstreams.emplace(&inRecord.getFace());
    }
  }

  for (const auto& pendingDownstream : pendingDownstreams) {
    this->sendData(data, *pendingDownstream, pitEntry);
  }
}

void
Strategy::sendNacks(const lp::NackHeader& header, const shared_ptr<pit::Entry>& pitEntry,
                    std::initializer_list<const Face*> exceptFaces)
{
  // populate downstreams with all downstreams faces
  std::unordered_set<Face*> downstreams;
  std::transform(pitEntry->in_begin(), pitEntry->in_end(),
                 std::inserter(downstreams, downstreams.end()),
                 [] (const auto& inR) { return &inR.getFace(); });

  // remove excluded faces
  for (auto exceptFace : exceptFaces) {
    downstreams.erase(const_cast<Face*>(exceptFace));
  }

  // send Nacks
  for (auto downstream : downstreams) {
    this->sendNack(header, *downstream, pitEntry);
  }
  // warning: don't loop on pitEntry->getInRecords(), because in-record is deleted when sending Nack
}

const fib::Entry&
Strategy::lookupFib(const pit::Entry& pitEntry) const
{
  const Fib& fib = m_forwarder.getFib();

  const Interest& interest = pitEntry.getInterest();
  // has forwarding hint?
  if (interest.getForwardingHint().empty()) {
    // FIB lookup with Interest name
    const fib::Entry& fibEntry = fib.findLongestPrefixMatch(pitEntry);
    NFD_LOG_TRACE("lookupFib noForwardingHint found=" << fibEntry.getPrefix());
    return fibEntry;
  }

  const auto& fh = interest.getForwardingHint();
  // Forwarding hint should have been stripped by incoming Interest pipeline when reaching producer region
  BOOST_ASSERT(!m_forwarder.getNetworkRegionTable().isInProducerRegion(fh));

  const fib::Entry* fibEntry = nullptr;
  for (const auto& delegation : fh) {
    fibEntry = &fib.findLongestPrefixMatch(delegation);
    if (fibEntry->hasNextHops()) {
      if (fibEntry->getPrefix().empty()) {
        // in consumer region, return the default route
        NFD_LOG_TRACE("lookupFib inConsumerRegion found=" << fibEntry->getPrefix());
      }
      else {
        // in default-free zone, use the first delegation that finds a FIB entry
        NFD_LOG_TRACE("lookupFib delegation=" << delegation << " found=" << fibEntry->getPrefix());
      }
      return *fibEntry;
    }
    BOOST_ASSERT(fibEntry->getPrefix().empty()); // only ndn:/ FIB entry can have zero nexthop
  }
  BOOST_ASSERT(fibEntry != nullptr && fibEntry->getPrefix().empty());
  return *fibEntry; // only occurs if no delegation finds a FIB nexthop
}

} // namespace nfd::fw
