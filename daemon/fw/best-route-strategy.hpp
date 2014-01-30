/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_BEST_ROUTE_STRATEGY_HPP
#define NFD_FW_BEST_ROUTE_STRATEGY_HPP

#include "strategy.hpp"

namespace nfd {

/** \class BestRouteStrategy
 *  \brief a forwarding strategy that forwards Interest
 *         to the first nexthop
 */
class BestRouteStrategy : public Strategy
{
public:
  explicit
  BestRouteStrategy(Forwarder& fw);
  
  virtual
  ~BestRouteStrategy();
  
  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry,
                       pit::InRecordCollection::iterator pitInRecord
                       );
};

} // namespace nfd

#endif // NFD_FW_BEST_ROUTE_STRATEGY_HPP
