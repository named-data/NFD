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

#include "strategy.hpp"
#include "forwarder.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("Strategy");

Strategy::Strategy(Forwarder& forwarder, const Name& name)
  : afterAddFace(forwarder.getFaceTable().afterAdd)
  , beforeRemoveFace(forwarder.getFaceTable().beforeRemove)
  , m_name(name)
  , m_forwarder(forwarder)
  , m_measurements(m_forwarder.getMeasurements(),
                   m_forwarder.getStrategyChoice(), *this)
{
}

Strategy::~Strategy()
{
}

void
Strategy::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,
                                const Face& inFace, const Data& data)
{
  NFD_LOG_DEBUG("beforeSatisfyInterest pitEntry=" << pitEntry->getName() <<
    " inFace=" << inFace.getId() << " data=" << data.getName());
}

void
Strategy::beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry)
{
  NFD_LOG_DEBUG("beforeExpirePendingInterest pitEntry=" << pitEntry->getName());
}

void
Strategy::afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                           shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{
  NFD_LOG_DEBUG("afterReceiveNack inFace=" << inFace.getId() <<
                " pitEntry=" << pitEntry->getName());
}

void
Strategy::sendNacks(shared_ptr<pit::Entry> pitEntry, const lp::NackHeader& header,
                    std::initializer_list<const Face*> exceptFaces)
{
  // populate downstreams with all downstreams faces
  std::unordered_set<const Face*> downstreams;
  std::transform(pitEntry->in_begin(), pitEntry->in_end(), std::inserter(downstreams, downstreams.end()),
                 [] (const pit::InRecord& inR) { return inR.getFace().get(); });

  // delete excluded faces
  // .erase in a loop is more efficient than std::set_difference between that requires sorted range
  for (const Face* exceptFace : exceptFaces) {
    downstreams.erase(exceptFace);
  }

  // send Nacks
  for (const Face* downstream : downstreams) {
    this->sendNack(pitEntry, *downstream, header);
  }
  // warning: don't loop on pitEntry->getInRecords(), because in-record is deleted when sending Nack
}

} // namespace fw
} // namespace nfd
