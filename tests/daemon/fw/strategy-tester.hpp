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

#ifndef NFD_TESTS_DAEMON_FW_STRATEGY_TESTER_HPP
#define NFD_TESTS_DAEMON_FW_STRATEGY_TESTER_HPP

#include "fw/strategy.hpp"

#include "tests/daemon/limited-io.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

/** \brief Extends strategy S for unit testing.
 *
 *  Actions invoked by S are recorded but not passed to forwarder.
 *
 *  StrategyTester should be registered into the strategy registry prior to use.
 *  \code
 *  // appears in or included by every .cpp MyStrategyTester is used
 *  using MyStrategyTester = StrategyTester<MyStrategy>;
 *
 *  // appears in only one .cpp
 *  NFD_REGISTER_STRATEGY(MyStrategyTester);
 *  \endcode
 */
template<typename S>
class StrategyTester : public S
{
public:
  using S::S;

  static Name
  getStrategyName()
  {
    Name name = S::getStrategyName();
    if (!name.empty() && name[-1].isVersion()) {
      // insert "tester" before version component
      name::Component versionComp = name[-1];
      name = name.getPrefix(-1);
      name.append("tester");
      name.append(versionComp);
    }
    else {
      name.append("tester");
    }
    return name;
  }

  /** \brief Signal emitted after each action
   */
  signal::Signal<StrategyTester<S>> afterAction;

  /** \brief execute f and wait for a number of strategy actions
   *  \note The actions may occur either during f() invocation or afterwards.
   *  \return whether expected number of actions have occurred
   */
  template<typename F>
  bool
  waitForAction(F&& f, LimitedIo& limitedIo, int nExpectedActions = 1)
  {
    int nActions = 0;

    signal::ScopedConnection conn = afterAction.connect([&] {
      limitedIo.afterOp();
      ++nActions;
    });

    std::forward<F>(f)();

    if (nActions < nExpectedActions) {
      // If strategy doesn't forward anything (e.g., decides not to forward an Interest), the number
      // of expected actions should be 0; otherwise the test will get stuck.
      return limitedIo.run(nExpectedActions - nActions, LimitedIo::UNLIMITED_TIME) == LimitedIo::EXCEED_OPS;
    }
    return nActions == nExpectedActions;
  }

protected:
  pit::OutRecord*
  sendInterest(const Interest& interest, Face& egress,
               const shared_ptr<pit::Entry>& pitEntry) override
  {
    sendInterestHistory.push_back({pitEntry->getInterest(), egress.getId(), interest});
    auto it = pitEntry->insertOrUpdateOutRecord(egress, interest);
    BOOST_ASSERT(it != pitEntry->out_end());
    afterAction();
    return &*it;
  }

  void
  rejectPendingInterest(const shared_ptr<pit::Entry>& pitEntry) override
  {
    rejectPendingInterestHistory.push_back({pitEntry->getInterest()});
    afterAction();
  }

  bool
  sendNack(const lp::NackHeader& header, Face& egress,
           const shared_ptr<pit::Entry>& pitEntry) override
  {
    sendNackHistory.push_back({pitEntry->getInterest(), egress.getId(), header});
    pitEntry->deleteInRecord(egress);
    afterAction();
    return true;
  }

public:
  struct SendInterestArgs
  {
    Interest pitInterest;
    FaceId outFaceId;
    Interest interest;
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

#endif // NFD_TESTS_DAEMON_FW_STRATEGY_TESTER_HPP
