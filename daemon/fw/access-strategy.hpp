/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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
#include "retx-suppression-fixed.hpp"

#include <ndn-cxx/util/rtt-estimator.hpp>

#include <unordered_map>

/* -------------------------------------------------------------------------
 * AccessStrategy.hpp の解説
 *
 * ■ 概要
 *   - NDN の「アクセスルータ向け」フォワーディング戦略
 *   - 最終ホップ（ラップトップ接続、ロスのあるリンク、正しい FIB 前提）向け設計
 *   - 基本動作：
 *       1. 初回 Interest は全 Nexthop にマルチキャスト
 *       2. Data が返ってきたら最後に成功した Nexthop を記録
 *       3. 次回 Interest は記録された Nexthop に送信
 *          返答がなければ再度マルチキャスト
 *
 * ■ 主なクラス
 *
 * AccessStrategy
 *   - Strategy から継承
 *   - afterReceiveInterest / beforeSatisfyInterest がトリガ
 *
 * PitInfo (PIT エントリに付随する StrategyInfo)
 *   - RTO タイマー保持
 *
 * MtInfo (Measurement Table 情報)
 *   - Prefix 単位で最後に成功した Nexthop を記録
 *   - RTT 推定器 (RttEstimator) を保持
 *
 * FaceInfo
 *   - グローバルな Face 単位の RTT 情報を保持
 *
 * ■ 主な処理
 *
 * afterReceiveNewInterest
 *   - 新しい Interest を受信した際の処理
 *
 * afterReceiveRetxInterest
 *   - 再送 Interest の処理
 *
 * sendToLastNexthop
 *   - 最後に成功した Nexthop に送信
 *
 * multicast
 *   - 全 Nexthop にマルチキャスト送信
 *
 * afterRtoTimeout
 *   - RTO タイムアウト時の再送処理
 *
 * updateMeasurements
 *   - Data 受信後、RTT と最後の成功 Nexthop を更新
 *
 * ■ 目的
 *   - ロスの多い最終ホップで安定して Interests を転送
 *   - 過去の成功情報を活用して効率的に転送
 *
 * ------------------------------------------------------------------------- */

namespace nfd::fw {

/**
 * \brief A forwarding strategy for "access" routers.
 *
 * This strategy is designed for the last hop on the NDN testbed,
 * where each nexthop connects to a laptop, links are lossy, and FIB is mostly correct.
 *
 * 1. Multicast the first Interest to all nexthops.
 * 2. When Data comes back, remember the last working nexthop of the prefix;
 *    the granularity of this knowledge is the parent of Data Name.
 * 3. Forward subsequent Interests to the last working nexthop.
 *    If there is no reply, multicast again (step 1).
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
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  beforeSatisfyInterest(const Data& data, const FaceEndpoint& ingress,
                        const shared_ptr<pit::Entry>& pitEntry) override;

private: // StrategyInfo
  using RttEstimator = ndn::util::RttEstimator;

  /** \brief StrategyInfo on PIT entry.
   */
  class PitInfo final : public StrategyInfo
  {
  public:
    static constexpr int
    getTypeId()
    {
      return 1010;
    }

  public:
    ndn::scheduler::ScopedEventId rtoTimer;
  };

  /** \brief StrategyInfo in measurements table.
   */
  class MtInfo final : public StrategyInfo
  {
  public:
    static constexpr int
    getTypeId()
    {
      return 1011;
    }

    explicit
    MtInfo(shared_ptr<const RttEstimator::Options> opts)
      : rtt(std::move(opts))
    {
    }

  public:
    FaceId lastNexthop = face::INVALID_FACEID;
    RttEstimator rtt;
  };

  /** \brief Find per-prefix measurements for Interest.
   */
  std::tuple<Name, MtInfo*>
  findPrefixMeasurements(const pit::Entry& pitEntry);

  /** \brief Get or create pre-prefix measurements for incoming Data.
   *  \note This function creates MtInfo but doesn't update it.
   */
  MtInfo*
  addPrefixMeasurements(const Data& data);

  /** \brief Global per-face StrategyInfo.
   *  \todo Currently stored inside AccessStrategy instance; should be moved
   *        to measurements table or somewhere else.
   */
  class FaceInfo
  {
  public:
    explicit
    FaceInfo(shared_ptr<const RttEstimator::Options> opts)
      : rtt(std::move(opts))
    {
    }

  public:
    RttEstimator rtt;
  };

private: // forwarding procedures
  void
  afterReceiveNewInterest(const Interest& interest, const FaceEndpoint& ingress,
                          const shared_ptr<pit::Entry>& pitEntry);

  void
  afterReceiveRetxInterest(const Interest& interest, const FaceEndpoint& ingress,
                           const shared_ptr<pit::Entry>& pitEntry);

  /** \brief Send to last working nexthop.
   *  \return whether an Interest is sent
   */
  bool
  sendToLastNexthop(const Interest& interest, const FaceEndpoint& ingress,
                    const shared_ptr<pit::Entry>& pitEntry, MtInfo& mi,
                    const fib::Entry& fibEntry);

  void
  afterRtoTimeout(const weak_ptr<pit::Entry>& pitWeak,
                  FaceId inFaceId, FaceId firstOutFaceId);

  /** \brief Multicast to all nexthops.
   *  \param exceptFace don't forward to this face; also, \p inFace is always excluded
   *  \return number of Interests that were sent
   */
  size_t
  multicast(const Interest& interest, const Face& inFace,
            const shared_ptr<pit::Entry>& pitEntry, const fib::Entry& fibEntry,
            FaceId exceptFace = face::INVALID_FACEID);

  void
  updateMeasurements(const Face& inFace, const Data& data, time::nanoseconds rtt);

private:
  const shared_ptr<const RttEstimator::Options> m_rttEstimatorOpts;
  std::unordered_map<FaceId, FaceInfo> m_fit;
  RetxSuppressionFixed m_retxSuppression;
  signal::ScopedConnection m_removeFaceConn;
};

} // namespace nfd::fw

#endif // NFD_DAEMON_FW_ACCESS_STRATEGY_HPP
