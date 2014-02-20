/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_FW_STRATEGY_TESTER_HPP
#define NFD_TEST_FW_STRATEGY_TESTER_HPP

#include <boost/tuple/tuple_comparison.hpp>
#include "fw/strategy.hpp"

namespace nfd {

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
    : S(forwarder)
  {
  }
  
  /// fires after each Action
  EventEmitter<> onAction;

protected:
  virtual void
  sendInterest(shared_ptr<pit::Entry> pitEntry,shared_ptr<Face> outFace);

  virtual void
  rejectPendingInterest(shared_ptr<pit::Entry> pitEntry);

public:
  typedef boost::tuple<shared_ptr<pit::Entry>, shared_ptr<Face> > SendInterestArgs;
  std::vector<SendInterestArgs> m_sendInterestHistory;

  typedef boost::tuple<shared_ptr<pit::Entry> > RejectPendingInterestArgs;
  std::vector<RejectPendingInterestArgs> m_rejectPendingInterestHistory;
};


template<typename S>
inline void
StrategyTester<S>::sendInterest(shared_ptr<pit::Entry> pitEntry,
                                shared_ptr<Face> outFace)
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


} // namespace nfd

#endif // TEST_FW_STRATEGY_TESTER_HPP
