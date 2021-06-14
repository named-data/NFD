/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
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
{
  this->setInstanceName(name);
}

void
DummyStrategy::afterReceiveInterest(const Interest& interest, const FaceEndpoint&,
                                    const shared_ptr<pit::Entry>& pitEntry)
{
  ++afterReceiveInterest_count;

  if (interestOutFace != nullptr) {
    this->sendInterest(interest, *interestOutFace, pitEntry);
  }
  else {
    this->rejectPendingInterest(pitEntry);
  }
}

void
DummyStrategy::afterContentStoreHit(const Data& data, const FaceEndpoint& ingress,
                                    const shared_ptr<pit::Entry>& pitEntry)
{
  ++afterContentStoreHit_count;

  Strategy::afterContentStoreHit(data, ingress, pitEntry);
}

void
DummyStrategy::beforeSatisfyInterest(const Data& data, const FaceEndpoint& ingress,
                                     const shared_ptr<pit::Entry>& pitEntry)
{
  ++beforeSatisfyInterest_count;

  Strategy::beforeSatisfyInterest(data, ingress, pitEntry);
}

void
DummyStrategy::afterReceiveData(const Data& data, const FaceEndpoint& ingress,
                                const shared_ptr<pit::Entry>& pitEntry)
{
  ++afterReceiveData_count;

  Strategy::afterReceiveData(data, ingress, pitEntry);
}

void
DummyStrategy::afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                                const shared_ptr<pit::Entry>& pitEntry)
{
  ++afterReceiveNack_count;

  Strategy::afterReceiveNack(nack, ingress, pitEntry);
}

void
DummyStrategy::afterNewNextHop(const fib::NextHop& nextHop, const shared_ptr<pit::Entry>& pitEntry)
{
  afterNewNextHopCalls.push_back(pitEntry->getName());

  Strategy::afterNewNextHop(nextHop, pitEntry);
}

} // namespace tests
} // namespace nfd
