/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_STRATEGY_HPP
#define NFD_FW_STRATEGY_HPP

#include "forwarder.hpp"

namespace nfd {

/** \class Strategy
 *  \brief represents a forwarding strategy
 */
class Strategy
{
public:
  explicit
  Strategy(Forwarder& fw);
  
  virtual
  ~Strategy();
  
  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry,
                       pit::InRecordCollection::iterator pitInRecord
                       ) =0;
  
  //virtual void
  //beforeExpirePendingInterest() =0;
  
  //virtual void
  //afterAddFibEntry() =0;
  
  //virtual void
  //afterUpdateFibEntry() =0;
  
  //virtual void
  //beforeRemoveFibEntry() =0;
  
protected: // actions
  /// send Interest to outFace
  void
  sendInterest(shared_ptr<pit::Entry> pitEntry,
                    shared_ptr<Face> outFace);
  
  /** \brief decide that a pending Interest cannot be forwarded
   *  This shall not be called if the pending Interest has been
   *  forwarded earlier, and does not need to be resent now.
   */
  void
  rebuffPendingInterest(shared_ptr<pit::Entry> pitEntry);
  
private:
  Forwarder& m_fw;
};

} // namespace nfd

#endif // NFD_FW_STRATEGY_HPP
