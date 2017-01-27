/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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
#include "tests/limited-io.hpp"

namespace nfd {
namespace fw {
namespace tests {

/** \brief extends strategy S for unit testing
 *
 *  Actions invoked by S are recorded but not passed to forwarder
 *
 *  StrategyTester should be registered into the strategy registry prior to use.
 *  \code
 *  // appears in or included by every .cpp MyStrategyTester is used
 *  typedef StrategyTester<MyStrategy> MyStrategyTester;
 *
 *  // appears in only one .cpp
 *  NFD_REGISTER_STRATEGY(MyStrategyTester);
 *  \endcode
 */
template<typename S>
class StrategyTester : public S
{
public:
  explicit
  StrategyTester(Forwarder& forwarder, const Name& name = getStrategyName())
    : S(forwarder, name)
  {
  }

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

  /** \brief signal emitted after each Action
   */
  signal::Signal<StrategyTester<S>> afterAction;

  /** \brief execute f and wait for a number of strategy actions
   *  \note The actions may occur either during f() invocation or afterwards.
   *  \return whether expected number of actions have occurred
   */
  bool
  waitForAction(const std::function<void()>& f,
                nfd::tests::LimitedIo& limitedIo, int nExpectedActions = 1)
  {
    int nActions = 0;

    signal::ScopedConnection conn = afterAction.connect([&] {
      limitedIo.afterOp();
      ++nActions;
    });

    f();

    if (nActions < nExpectedActions) {
      // A correctly implemented strategy is required to invoke reject pending Interest action if it
      // decides to not forward an Interest. If a test case is stuck in the call below, check that
      // rejectPendingInterest is invoked under proper condition.
      return limitedIo.run(nExpectedActions - nActions, nfd::tests::LimitedIo::UNLIMITED_TIME) ==
             nfd::tests::LimitedIo::EXCEED_OPS;
    }
    return nActions == nExpectedActions;
  }

protected:
  void
  sendInterest(const shared_ptr<pit::Entry>& pitEntry, Face& outFace,
               const Interest& interest) override
  {
    sendInterestHistory.push_back({pitEntry->getInterest(), outFace.getId(), interest});
    pitEntry->insertOrUpdateOutRecord(outFace, interest);
    afterAction();
  }

  void
  rejectPendingInterest(const shared_ptr<pit::Entry>& pitEntry) override
  {
    rejectPendingInterestHistory.push_back({pitEntry->getInterest()});
    afterAction();
  }

  void
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
