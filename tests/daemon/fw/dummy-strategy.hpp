/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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
 *  Unless otherwise indicated, triggers are recorded but does nothing.
 */
class DummyStrategy : public fw::Strategy
{
public:
  DummyStrategy(Forwarder& forwarder, const Name& name)
    : Strategy(forwarder, name)
    , afterReceiveInterest_count(0)
    , wantAfterReceiveInterestCalls(false)
    , beforeSatisfyInterest_count(0)
    , beforeExpirePendingInterest_count(0)
  {
  }

  /** \brief after receive Interest trigger
   *
   *  If \p interestOutFace is not null, send Interest action is invoked with that face;
   *  otherwise, reject pending Interest action is invoked.
   */
  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE
  {
    ++afterReceiveInterest_count;
    if (wantAfterReceiveInterestCalls) {
      afterReceiveInterestCalls.push_back(std::make_tuple(inFace.getId(),
        interest, fibEntry, pitEntry));
    }

    if (interestOutFace) {
      this->sendInterest(pitEntry, interestOutFace);
    }
    else {
      this->rejectPendingInterest(pitEntry);
    }
  }

  virtual void
  beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,
                        const Face& inFace, const Data& data) DECL_OVERRIDE
  {
    ++beforeSatisfyInterest_count;
  }

  virtual void
  beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE
  {
    ++beforeExpirePendingInterest_count;
  }

  virtual void
  afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                   shared_ptr<fib::Entry> fibEntry,
                   shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE
  {
    ++afterReceiveNack_count;
  }

public:
  int afterReceiveInterest_count;
  bool wantAfterReceiveInterestCalls;
  std::vector<std::tuple<FaceId, Interest, shared_ptr<fib::Entry>,
              shared_ptr<pit::Entry>>> afterReceiveInterestCalls;
  shared_ptr<Face> interestOutFace;

  int beforeSatisfyInterest_count;
  int beforeExpirePendingInterest_count;
  int afterReceiveNack_count;

};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FW_DUMMY_STRATEGY_HPP
