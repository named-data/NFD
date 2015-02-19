/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

/** \class StrategyTester
 *  \brief extends strategy S for unit testing
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
  signal::Signal<StrategyTester<S>> onAction;

protected:
  virtual void
  sendInterest(shared_ptr<pit::Entry> pitEntry,
               shared_ptr<Face> outFace,
               bool wantNewNonce = false) DECL_OVERRIDE;

  virtual void
  rejectPendingInterest(shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE;

public:
  typedef boost::tuple<shared_ptr<pit::Entry>, shared_ptr<Face>> SendInterestArgs;
  std::vector<SendInterestArgs> m_sendInterestHistory;

  typedef boost::tuple<shared_ptr<pit::Entry>> RejectPendingInterestArgs;
  std::vector<RejectPendingInterestArgs> m_rejectPendingInterestHistory;
};


template<typename S>
inline void
StrategyTester<S>::sendInterest(shared_ptr<pit::Entry> pitEntry,
                                shared_ptr<Face> outFace,
                                bool wantNewNonce)
{
  m_sendInterestHistory.push_back(SendInterestArgs(pitEntry, outFace));
  pitEntry->insertOrUpdateOutRecord(outFace, pitEntry->getInterest());
  onAction();
}

template<typename S>
inline void
StrategyTester<S>::rejectPendingInterest(shared_ptr<pit::Entry> pitEntry)
{
  m_rejectPendingInterestHistory.push_back(RejectPendingInterestArgs(pitEntry));
  onAction();
}

} // namespace tests
} // namespace fw
} // namespace nfd

#endif // NFD_TESTS_NFD_FW_STRATEGY_TESTER_HPP
