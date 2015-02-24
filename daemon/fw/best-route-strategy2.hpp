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

#ifndef NFD_DAEMON_FW_BEST_ROUTE_STRATEGY2_HPP
#define NFD_DAEMON_FW_BEST_ROUTE_STRATEGY2_HPP

#include "strategy.hpp"
#include "retx-suppression-exponential.hpp"

namespace nfd {
namespace fw {

/** \brief Best Route strategy version 3
 *
 *  This strategy forwards a new Interest to the lowest-cost nexthop (except downstream).
 *  After that, if consumer retransmits the Interest (and is not suppressed according to
 *  exponential backoff algorithm), the strategy forwards the Interest again to
 *  the lowest-cost nexthop (except downstream) that is not previously used.
 *  If all nexthops have been used, the strategy starts over.
 */
class BestRouteStrategy2 : public Strategy
{
public:
  BestRouteStrategy2(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE;

public:
  static const Name STRATEGY_NAME;

private:
  RetxSuppressionExponential m_retxSuppression;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_BEST_ROUTE_STRATEGY2_HPP
