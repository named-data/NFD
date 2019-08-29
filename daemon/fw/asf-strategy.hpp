/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_ASF_STRATEGY_HPP
#define NFD_DAEMON_FW_ASF_STRATEGY_HPP

#include "asf-measurements.hpp"
#include "asf-probing-module.hpp"
#include "fw/retx-suppression-exponential.hpp"
#include "fw/strategy.hpp"

namespace nfd {
namespace fw {
namespace asf {

/** \brief Adaptive SRTT-based Forwarding Strategy
 *
 *  \see Vince Lehman, Ashlesh Gawande, Rodrigo Aldecoa, Dmitri Krioukov, Beichuan Zhang, Lixia Zhang, and Lan Wang,
 *       "An Experimental Investigation of Hyperbolic Routing with a Smart Forwarding Plane in NDN,"
 *       NDN Technical Report NDN-0042, 2016. http://named-data.net/techreports.html
 *
 *  \note This strategy is not EndpointId-aware.
 */
class AsfStrategy : public Strategy
{
public:
  explicit
  AsfStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

public: // triggers
  void
  afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const FaceEndpoint& ingress, const Data& data) override;

  void
  afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry) override;

private:
  void
  processParams(const PartialName& parsed);

  void
  forwardInterest(const Interest& interest, Face& outFace, const fib::Entry& fibEntry,
                  const shared_ptr<pit::Entry>& pitEntry, bool wantNewNonce = false);

  void
  sendProbe(const Interest& interest, const FaceEndpoint& ingress, const Face& faceToUse,
            const fib::Entry& fibEntry, const shared_ptr<pit::Entry>& pitEntry);

  Face*
  getBestFaceForForwarding(const Interest& interest, const Face& inFace,
                           const fib::Entry& fibEntry, const shared_ptr<pit::Entry>& pitEntry,
                           bool isNewInterest = true);

  void
  onTimeout(const Name& interestName, FaceId faceId);

  void
  sendNoRouteNack(const FaceEndpoint& ingress, const shared_ptr<pit::Entry>& pitEntry);

private:
  AsfMeasurements m_measurements;
  ProbingModule m_probing;
  RetxSuppressionExponential m_retxSuppression;
  size_t m_maxSilentTimeouts = 0;

  static const time::milliseconds RETX_SUPPRESSION_INITIAL;
  static const time::milliseconds RETX_SUPPRESSION_MAX;
};

} // namespace asf

using asf::AsfStrategy;

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_ASF_STRATEGY_HPP
