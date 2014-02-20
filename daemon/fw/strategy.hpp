/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_STRATEGY_HPP
#define NFD_FW_STRATEGY_HPP

#include "face/face.hpp"

namespace nfd {

class Forwarder;
namespace fib {
class Entry;
}
namespace pit {
class Entry;
}

namespace fw {

/** \class Strategy
 *  \brief represents a forwarding strategy
 */
class Strategy
{
public:
  explicit
  Strategy(Forwarder& forwarder);
  
  virtual
  ~Strategy();
  
public: // triggers
  /** \brief trigger after Interest is received
   *
   *  The Interest:
   *  - does not violate Scope
   *  - is not looped
   *  - cannot be satisfied by ContentStore
   *  - is under a namespace managed by this strategy
   *
   *  The strategy should decide whether and where to forward this Interest.
   *  - If the strategy decides to forward this Interest,
   *    invoke this->sendInterest one or more times, either now or shortly after
   *  - If strategy concludes that this Interest cannot be forwarded,
   *    invoke this->rebuffPendingInterest so that PIT entry will be deleted shortly
   */
  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry) =0;
  
  /** \brief trigger before PIT entry is satisfied
   *
   *  In this base class this method does nothing.
   */
  virtual void
  beforeSatisfyPendingInterest(shared_ptr<pit::Entry> pitEntry,
                               const Face& inFace, const Data& data);
  
  /** \brief trigger before PIT entry expires
   *
   *  PIT entry expires when InterestLifetime has elapsed for all InRecords,
   *  and it is not satisfied by an incoming Data.
   *
   *  This trigger is not invoked for PIT entry already satisfied.
   *
   *  In this base class this method does nothing.
   */
  virtual void
  beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry);
  
//  /** \brief trigger after FIB entry is being inserted
//   *         and becomes managed by this strategy
//   *
//   *  In this base class this method does nothing.
//   */
//  virtual void
//  afterAddFibEntry(shared_ptr<fib::Entry> fibEntry);
//  
//  /** \brief trigger after FIB entry being managed by this strategy is updated
//   *
//   *  In this base class this method does nothing.
//   */
//  virtual void
//  afterUpdateFibEntry(shared_ptr<fib::Entry> fibEntry);
//  
//  /** \brief trigger before FIB entry ceises to be managed by this strategy
//   *         or is being deleted
//   *
//   *  In this base class this method does nothing.
//   */
//  virtual void
//  beforeRemoveFibEntry(shared_ptr<fib::Entry> fibEntry);
  
protected: // actions
  /// send Interest to outFace
  VIRTUAL_WITH_TESTS void
  sendInterest(shared_ptr<pit::Entry> pitEntry,
                    shared_ptr<Face> outFace);
  
  /** \brief decide that a pending Interest cannot be forwarded
   *
   *  This shall not be called if the pending Interest has been
   *  forwarded earlier, and does not need to be resent now.
   */
  VIRTUAL_WITH_TESTS void
  rebuffPendingInterest(shared_ptr<pit::Entry> pitEntry);
  
private:
  /** \brief reference to the forwarder
   *
   *  Triggers can access forwarder indirectly via actions.
   */
  Forwarder& m_forwarder;
};

} // namespace fw
} // namespace nfd

#endif // NFD_FW_STRATEGY_HPP
