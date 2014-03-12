/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_FW_DUMMY_STRATEGY_HPP
#define NFD_TEST_FW_DUMMY_STRATEGY_HPP

#include "fw/strategy.hpp"

namespace nfd {
namespace tests {

/** \brief strategy for unit testing
 *
 *  Triggers on DummyStrategy are recorded but does nothing
 */
class DummyStrategy : public fw::Strategy
{
public:
  DummyStrategy(Forwarder& forwarder, const Name& name)
    : Strategy(forwarder, name)
  {
  }
  
  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry)
  {
    ++m_afterReceiveInterest_count;
    
    if (static_cast<bool>(m_interestOutFace)) {
      this->sendInterest(pitEntry, m_interestOutFace);
    }
    else {
      this->rejectPendingInterest(pitEntry);
    }
  }
  
  virtual void
  beforeSatisfyPendingInterest(shared_ptr<pit::Entry> pitEntry,
                               const Face& inFace, const Data& data)
  {
    ++m_beforeSatisfyPendingInterest_count;
  }

  virtual void
  beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry)
  {
    ++m_beforeExpirePendingInterest_count;
  }

public:
  int m_afterReceiveInterest_count;
  int m_beforeSatisfyPendingInterest_count;
  int m_beforeExpirePendingInterest_count;
  
  /// outFace to use in afterReceiveInterest, nullptr to reject
  shared_ptr<Face> m_interestOutFace;
};

} // namespace tests
} // namespace nfd

#endif // TEST_FW_DUMMY_STRATEGY_HPP
