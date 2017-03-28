/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_ACCESS_STRATEGY_HPP
#define NFD_DAEMON_FW_ACCESS_STRATEGY_HPP

#include "strategy.hpp"
#include "core/rtt-estimator.hpp"
#include "retx-suppression-fixed.hpp"
#include <unordered_set>
#include <unordered_map>

namespace nfd {
namespace fw {

/** \brief Access Router Strategy version 1
 *
 *  This strategy is designed for the last hop on the NDN testbed,
 *  where each nexthop connects to a laptop, links are lossy, and FIB is mostly correct.
 *
 *  1. Multicast the first Interest to all nexthops.
 *  2. When Data comes back, remember last working nexthop of the prefix;
 *     the granularity of this knowledge is the parent of Data Name.
 *  3. Forward subsequent Interests to the last working nexthop.
 *     If it doesn't respond, multicast again.
 */
class AccessStrategy : public Strategy
{
public:
  explicit
  AccessStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

public: // triggers
  void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const Face& inFace, const Data& data) override;

private: // StrategyInfo
  /** \brief StrategyInfo on PIT entry
   */
  class PitInfo : public StrategyInfo
  {
  public:
    static constexpr int
    getTypeId()
    {
      return 1010;
    }

  public:
    scheduler::ScopedEventId rtoTimer;
  };

  /** \brief StrategyInfo in measurements table
   */
  class MtInfo : public StrategyInfo
  {
  public:
    static constexpr int
    getTypeId()
    {
      return 1011;
    }

    MtInfo();

  public:
    FaceId lastNexthop;
    RttEstimator rtt;
  };

  /** \brief find per-prefix measurements for Interest
   */
  std::tuple<Name, MtInfo*>
  findPrefixMeasurements(const pit::Entry& pitEntry);

  /** \brief get or create pre-prefix measurements for incoming Data
   *  \note This function creates MtInfo but doesn't update it.
   */
  MtInfo*
  addPrefixMeasurements(const Data& data);

  /** \brief global per-face StrategyInfo
   */
  class FaceInfo
  {
  public:
    FaceInfo();

  public:
    RttEstimator rtt;
  };

  typedef std::unordered_map<FaceId, FaceInfo> FaceInfoTable;

  void
  removeFaceInfo(const Face& face);

private: // forwarding procedures
  void
  afterReceiveNewInterest(const Face& inFace, const Interest& interest,
                          const shared_ptr<pit::Entry>& pitEntry);

  void
  afterReceiveRetxInterest(const Face& inFace, const Interest& interest,
                           const shared_ptr<pit::Entry>& pitEntry);

  /** \brief send to last working nexthop
   *  \return whether an Interest is sent
   */
  bool
  sendToLastNexthop(const Face& inFace, const Interest& interest,
                    const shared_ptr<pit::Entry>& pitEntry, MtInfo& mi,
                    const fib::Entry& fibEntry);

  void
  afterRtoTimeout(weak_ptr<pit::Entry> pitWeak, FaceId inFaceId, FaceId firstOutFaceId);

  /** \brief multicast to all nexthops
   *  \param exceptFace don't forward to this face; also, inFace is always excluded
   *  \return how many Interests are sent
   */
  int
  multicast(const Face& inFace, const Interest& interest,
            const shared_ptr<pit::Entry>& pitEntry, const fib::Entry& fibEntry,
            FaceId exceptFace = face::INVALID_FACEID);

  void
  updateMeasurements(const Face& inFace, const Data& data,
                     const RttEstimator::Duration& rtt);

private:
  FaceInfoTable m_fit;
  RetxSuppressionFixed m_retxSuppression;
  signal::ScopedConnection m_removeFaceInfoConn;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_ACCESS_STRATEGY_HPP
