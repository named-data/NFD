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

#include "strategy.hpp"
#include "forwarder.hpp"
#include "core/logger.hpp"
#include "core/random.hpp"
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace nfd {
namespace fw {

NFD_LOG_INIT("Strategy");

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
  NFD_LOG_DEBUG("create " << instanceName << " found=" << found->first <<
                " created=" << instance->getInstanceName());
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
  return {input, nullopt, PartialName()};
}

Name
Strategy::makeInstanceName(const Name& input, const Name& strategyName)
{
  BOOST_ASSERT(strategyName.at(-1).isVersion());

  bool hasVersion = std::any_of(input.rbegin(), input.rend(),
                                [] (const name::Component& comp) { return comp.isVersion(); });
  return hasVersion ? input : Name(input).append(strategyName.at(-1));
}

Strategy::Strategy(Forwarder& forwarder)
  : afterAddFace(forwarder.getFaceTable().afterAdd)
  , beforeRemoveFace(forwarder.getFaceTable().beforeRemove)
  , m_forwarder(forwarder)
  , m_measurements(m_forwarder.getMeasurements(), m_forwarder.getStrategyChoice(), *this)
{
}

Strategy::~Strategy() = default;

void
Strategy::beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                                const Face& inFace, const Data& data)
{
  NFD_LOG_DEBUG("beforeSatisfyInterest pitEntry=" << pitEntry->getName() <<
                " inFace=" << inFace.getId() << " data=" << data.getName());
}

void
Strategy::afterContentStoreHit(const shared_ptr<pit::Entry>& pitEntry,
                               const Face& inFace, const Data& data)
{
  NFD_LOG_DEBUG("afterContentStoreHit pitEntry=" << pitEntry->getName() <<
                " inFace=" << inFace.getId() << " data=" << data.getName());

  this->sendData(pitEntry, data, inFace);
}

void
Strategy::afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,
                           const Face& inFace, const Data& data)
{
  NFD_LOG_DEBUG("afterReceiveData pitEntry=" << pitEntry->getName() <<
                " inFace=" << inFace.getId() << " data=" << data.getName());

  this->beforeSatisfyInterest(pitEntry, inFace, data);

  this->sendDataToAll(pitEntry, inFace, data);
}

void
Strategy::afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                           const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("afterReceiveNack inFace=" << inFace.getId() <<
                " pitEntry=" << pitEntry->getName());
}

void
Strategy::onDroppedInterest(const Face& outFace, const Interest& interest)
{
  NFD_LOG_DEBUG("onDroppedInterest outFace=" << outFace.getId() << " name=" << interest.getName());
}

void
Strategy::sendData(const shared_ptr<pit::Entry>& pitEntry, const Data& data, const Face& outFace)
{
  BOOST_ASSERT(pitEntry->getInterest().matchesData(data));

  // delete the PIT entry's in-record based on outFace,
  // since Data is sent to outFace from which the Interest was received
  pitEntry->deleteInRecord(outFace);

  m_forwarder.onOutgoingData(data, *const_pointer_cast<Face>(outFace.shared_from_this()));
}

void
Strategy::sendDataToAll(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace, const Data& data)
{
  std::set<Face*> pendingDownstreams;
  auto now = time::steady_clock::now();

  // remember pending downstreams
  for (const pit::InRecord& inRecord : pitEntry->getInRecords()) {
    if (inRecord.getExpiry() > now) {
      if (inRecord.getFace().getId() == inFace.getId() &&
          inRecord.getFace().getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) {
        continue;
      }
      pendingDownstreams.insert(&inRecord.getFace());
    }
  }

  for (const Face* pendingDownstream : pendingDownstreams) {
    this->sendData(pitEntry, data, *pendingDownstream);
  }
}

void
Strategy::sendNacks(const shared_ptr<pit::Entry>& pitEntry, const lp::NackHeader& header,
                    std::initializer_list<const Face*> exceptFaces)
{
  // populate downstreams with all downstreams faces
  std::unordered_set<const Face*> downstreams;
  std::transform(pitEntry->in_begin(), pitEntry->in_end(), std::inserter(downstreams, downstreams.end()),
                 [] (const pit::InRecord& inR) { return &inR.getFace(); });

  // delete excluded faces
  // .erase in a loop is more efficient than std::set_difference because that requires sorted range
  for (const Face* exceptFace : exceptFaces) {
    downstreams.erase(exceptFace);
  }

  // send Nacks
  for (const Face* downstream : downstreams) {
    this->sendNack(pitEntry, *downstream, header);
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

  const DelegationList& fh = interest.getForwardingHint();
  // Forwarding hint should have been stripped by incoming Interest pipeline when reaching producer region
  BOOST_ASSERT(!m_forwarder.getNetworkRegionTable().isInProducerRegion(fh));

  const fib::Entry* fibEntry = nullptr;
  for (const Delegation& del : fh) {
    fibEntry = &fib.findLongestPrefixMatch(del.name);
    if (fibEntry->hasNextHops()) {
      if (fibEntry->getPrefix().size() == 0) {
        // in consumer region, return the default route
        NFD_LOG_TRACE("lookupFib inConsumerRegion found=" << fibEntry->getPrefix());
      }
      else {
        // in default-free zone, use the first delegation that finds a FIB entry
        NFD_LOG_TRACE("lookupFib delegation=" << del.name << " found=" << fibEntry->getPrefix());
      }
      return *fibEntry;
    }
    BOOST_ASSERT(fibEntry->getPrefix().size() == 0); // only ndn:/ FIB entry can have zero nexthop
  }
  BOOST_ASSERT(fibEntry != nullptr && fibEntry->getPrefix().size() == 0);
  return *fibEntry; // only occurs if no delegation finds a FIB nexthop
}

} // namespace fw
} // namespace nfd
