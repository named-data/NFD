/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_BROADCAST_STRATEGY_HPP
#define NFD_FW_BROADCAST_STRATEGY_HPP

#include "strategy.hpp"
#include "forwarder.hpp"

namespace nfd {
namespace fw {

/** \class BroadcastStrategy
 *  \brief a forwarding strategy that forwards Interest
 *         to all nexthops
 */
class BroadcastStrategy : public Strategy
{
public:
  explicit
  BroadcastStrategy(Forwarder& forwarder);
  
  virtual
  ~BroadcastStrategy();
  
  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry);
};

} // namespace fw
} // namespace nfd

#endif // NFD_FW_BROADCAST_STRATEGY_HPP
