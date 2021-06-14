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

#ifndef NFD_DAEMON_FW_BEST_ROUTE_STRATEGY_HPP
#define NFD_DAEMON_FW_BEST_ROUTE_STRATEGY_HPP

#include "strategy.hpp"
#include "process-nack-traits.hpp"
#include "retx-suppression-exponential.hpp"

namespace nfd {
namespace fw {

/** \brief Best Route strategy
 *
 *  This strategy forwards a new Interest to the lowest-cost nexthop (except downstream).
 *  After that, if consumer retransmits the Interest (and is not suppressed according to
 *  exponential backoff algorithm), the strategy forwards the Interest again to
 *  the lowest-cost nexthop (except downstream) that is not previously used.
 *  If all nexthops have been used, the strategy starts over with the first nexthop.
 *
 *  This strategy returns Nack to all downstreams with reason NoRoute
 *  if there is no usable nexthop, which may be caused by:
 *  (a) the FIB entry contains no nexthop;
 *  (b) the FIB nexthop happens to be the sole downstream;
 *  (c) the FIB nexthops violate scope.
 *
 *  This strategy returns Nack to all downstreams if all upstreams have returned Nacks.
 *  The reason of the sent Nack equals the least severe reason among received Nacks.
 */
class BestRouteStrategy : public Strategy
                        , public ProcessNackTraits<BestRouteStrategy>
{
public:
  explicit
  BestRouteStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

public: // triggers
  void
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry) override;

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  static const time::milliseconds RETX_SUPPRESSION_INITIAL;
  static const time::milliseconds RETX_SUPPRESSION_MAX;
  RetxSuppressionExponential m_retxSuppression;

  friend ProcessNackTraits<BestRouteStrategy>;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_BEST_ROUTE_STRATEGY_HPP
