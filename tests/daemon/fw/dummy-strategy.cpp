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

#include "dummy-strategy.hpp"

namespace nfd {
namespace tests {

NFD_REGISTER_STRATEGY(DummyStrategy);

void
DummyStrategy::registerAs(const Name& strategyName)
{
  registerAsImpl<DummyStrategy>(strategyName);
}

Name
DummyStrategy::getStrategyName(uint64_t version)
{
  return Name("/dummy-strategy").appendVersion(version);
}

DummyStrategy::DummyStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , afterReceiveInterest_count(0)
  , beforeSatisfyInterest_count(0)
  , afterContentStoreHit_count(0)
  , afterReceiveData_count(0)
  , afterReceiveNack_count(0)
{
  this->setInstanceName(name);
}

void
DummyStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                    const shared_ptr<pit::Entry>& pitEntry)
{
  ++afterReceiveInterest_count;

  if (interestOutFace != nullptr) {
    this->sendInterest(pitEntry, *interestOutFace, interest);
  }
  else {
    this->rejectPendingInterest(pitEntry);
  }
}

void
DummyStrategy::beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                                     const Face& inFace, const Data& data)
{
  ++beforeSatisfyInterest_count;
}

void
DummyStrategy::afterContentStoreHit(const shared_ptr<pit::Entry>& pitEntry,
                                    const Face& inFace, const Data& data)
{
  ++afterContentStoreHit_count;

  Strategy::afterContentStoreHit(pitEntry, inFace, data);
}

void
DummyStrategy::afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,
                                const Face& inFace, const Data& data)
{
  ++afterReceiveData_count;

  Strategy::afterReceiveData(pitEntry, inFace, data);
}

void
DummyStrategy::afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                                const shared_ptr<pit::Entry>& pitEntry)
{
  ++afterReceiveNack_count;
}

} // namespace tests
} // namespace nfd
