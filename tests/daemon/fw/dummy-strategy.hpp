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

#ifndef NFD_TESTS_DAEMON_FW_DUMMY_STRATEGY_HPP
#define NFD_TESTS_DAEMON_FW_DUMMY_STRATEGY_HPP

#include "fw/strategy.hpp"

namespace nfd {
namespace tests {

/** \brief strategy for unit testing
 *
 *  Triggers are recorded but do nothing.
 *
 *  DummyStrategy registers itself as /dummy-strategy/<max-version>, so that it can be instantiated
 *  with any version number. Aliases can be created with \p registerAs function.
 */
class DummyStrategy : public fw::Strategy
{
public:
  static void
  registerAs(const Name& strategyName);

  static Name
  getStrategyName(uint64_t version = std::numeric_limits<uint64_t>::max());

  /** \brief constructor
   *
   *  \p name is recorded unchanged as \p getInstanceName() , and will not automatically
   *  gain a version number when instantiated without a version number.
   */
  explicit
  DummyStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  /** \brief after receive Interest trigger
   *
   *  If \p interestOutFace is not null, Interest is forwarded to that face via send Interest action;
   *  otherwise, reject pending Interest action is invoked.
   */
  void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const Face& inFace, const Data& data) override;

  void
  afterContentStoreHit(const shared_ptr<pit::Entry>& pitEntry,
                       const Face& inFace, const Data& data) override;

  void
  afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,
                   const Face& inFace, const Data& data) override;

  void
  afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry) override;

protected:
  /** \brief register an alias
   *  \tparam S subclass of DummyStrategy
   */
  template<typename S>
  static void
  registerAsImpl(const Name& strategyName)
  {
    if (!fw::Strategy::canCreate(strategyName)) {
      fw::Strategy::registerType<S>(strategyName);
    }
  }

public:
  int afterReceiveInterest_count;
  int beforeSatisfyInterest_count;
  int afterContentStoreHit_count;
  int afterReceiveData_count;
  int afterReceiveNack_count;

  shared_ptr<Face> interestOutFace;
};

/** \brief DummyStrategy with specific version
 */
template<uint64_t VERSION>
class VersionedDummyStrategy : public DummyStrategy
{
public:
  static void
  registerAs(const Name& strategyName)
  {
    DummyStrategy::registerAsImpl<VersionedDummyStrategy<VERSION>>(strategyName);
  }

  static Name
  getStrategyName()
  {
    return DummyStrategy::getStrategyName(VERSION);
  }

  /** \brief constructor
   *
   *  The strategy instance name is taken from \p name ; if it does not contain a version component,
   *  \p VERSION will be appended.
   */
  explicit
  VersionedDummyStrategy(Forwarder& forwarder, const Name& name = getStrategyName())
    : DummyStrategy(forwarder, Strategy::makeInstanceName(name, getStrategyName()))
  {
  }
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FW_DUMMY_STRATEGY_HPP
