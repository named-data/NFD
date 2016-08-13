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

#ifndef NFD_TESTS_NFD_FW_STRATEGY_TESTER_HPP
#define NFD_TESTS_NFD_FW_STRATEGY_TESTER_HPP

#include <boost/tuple/tuple_comparison.hpp>
#include "fw/strategy.hpp"

namespace nfd {
namespace fw {
namespace tests {

/** \brief extends strategy S for unit testing
 *
 *  Actions invoked by S are recorded but not passed to forwarder
 */
template<typename S>
class StrategyTester : public S
{
public:
  explicit
  StrategyTester(Forwarder& forwarder)
    : S(forwarder, Name(S::STRATEGY_NAME).append("tester"))
  {
  }

  /// fires after each Action
  signal::Signal<StrategyTester<S>> afterAction;

protected:
  virtual void
  sendInterest(const shared_ptr<pit::Entry>& pitEntry,
               Face& outFace,
               bool wantNewNonce = false) override
  {
    sendInterestHistory.push_back({pitEntry->getInterest(), outFace.getId(), wantNewNonce});
    pitEntry->insertOrUpdateOutRecord(outFace, pitEntry->getInterest());
    afterAction();
  }

  virtual void
  rejectPendingInterest(const shared_ptr<pit::Entry>& pitEntry) override
  {
    rejectPendingInterestHistory.push_back({pitEntry->getInterest()});
    afterAction();
  }

  virtual void
  sendNack(const shared_ptr<pit::Entry>& pitEntry, const Face& outFace,
           const lp::NackHeader& header) override
  {
    sendNackHistory.push_back({pitEntry->getInterest(), outFace.getId(), header});
    pitEntry->deleteInRecord(outFace);
    afterAction();
  }

public:
  struct SendInterestArgs
  {
    Interest pitInterest;
    FaceId outFaceId;
    bool wantNewNonce;
  };
  std::vector<SendInterestArgs> sendInterestHistory;

  struct RejectPendingInterestArgs
  {
    Interest pitInterest;
  };
  std::vector<RejectPendingInterestArgs> rejectPendingInterestHistory;

  struct SendNackArgs
  {
    Interest pitInterest;
    FaceId outFaceId;
    lp::NackHeader header;
  };
  std::vector<SendNackArgs> sendNackHistory;
};

} // namespace tests
} // namespace fw
} // namespace nfd

#endif // NFD_TESTS_NFD_FW_STRATEGY_TESTER_HPP
