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

#ifndef NFD_DAEMON_FW_ASF_STRATEGY_HPP
#define NFD_DAEMON_FW_ASF_STRATEGY_HPP

#include "strategy.hpp"
#include "asf-measurements.hpp"
#include "asf-probing-module.hpp"
#include "process-nack-traits.hpp"
#include "retx-suppression-exponential.hpp"

/* -------------------------------------------------------------------------
 * AsfStrategy.hpp の解説
 *
 * ■ 概要
 *   - ASF（Adaptive SRTT-based Forwarding）は、各 nexthop の RTT（ラウンドトリップ時間）に基づき
 *     最適な経路を選択する戦略。
 *   - Nack やタイムアウト、再送抑制（指数バックオフ）を組み合わせて
 *     効率的な Interest 転送を行う。
 *   - プローブ機能を使ってまだ未知の nexthop のパフォーマンスも評価する。
 *
 * ■ クラス構造
 *
 * AsfStrategy : public Strategy, ProcessNackTraits<AsfStrategy>
 *   - コンストラクタで Forwarder と戦略名を受け取る
 *   - static getStrategyName() で戦略名を返す
 *
 * トリガーメソッド:
 *   - afterReceiveInterest()
 *       -> Interest 受信時に最適経路を選択して転送
 *
 *   - beforeSatisfyInterest()
 *       -> Data を PIT に返す直前に呼ばれる
 *       -> RTT 測定や統計更新に利用
 *
 *   - afterReceiveNack()
 *       -> upstream から Nack を受信した際に呼ばれる
 *       -> ProcessNackTraits で downstream に返す
 *
 * 内部メソッド:
 *   - forwardInterest()       : Interest を選択した Face に転送
 *   - sendProbe()             : 未知 nexthop の RTT を測定
 *   - getBestFaceForForwarding(): 最適な Face を選択
 *   - onTimeoutOrNack()       : タイムアウトや Nack を受信した場合の処理
 *   - sendNoRouteNack()       : 利用可能な経路がない場合 Nack を送信
 *
 * メンバ:
 *   - AsfMeasurements m_measurements   : Face ごとの RTT 等の統計情報管理
 *   - std::unique_ptr<RetxSuppressionExponential> m_retxSuppression
 *       -> 再送抑制アルゴリズム（指数バックオフ）
 *   - ProbingModule m_probing           : 未知 nexthop の評価用プローブ
 *   - size_t m_nMaxTimeouts             : 最大タイムアウト回数
 *
 * ■ 特徴
 *   - RTT に基づく動的経路選択でレイテンシを最小化
 *   - 再送抑制と Nack 処理で無駄な転送を削減
 *   - プローブで未知経路も評価して学習
 *
 * ■ 参考
 *   - Vince Lehman et al., "An Experimental Investigation of Hyperbolic Routing
 *     with a Smart Forwarding Plane in NDN", NDN Technical Report NDN-0042, 2016
 * ------------------------------------------------------------------------- */

namespace nfd::fw {
namespace asf {

/**
 * \brief Adaptive SRTT-based forwarding strategy.
 *
 * \see Vince Lehman, Ashlesh Gawande, Rodrigo Aldecoa, Dmitri Krioukov, Beichuan Zhang,
 *      Lixia Zhang, and Lan Wang, "An Experimental Investigation of Hyperbolic Routing
 *      with a Smart Forwarding Plane in NDN", NDN Technical Report NDN-0042, 2016.
 *      https://named-data.net/publications/techreports/ndn-0042-1-asf/
 */
class AsfStrategy : public Strategy, public ProcessNackTraits<AsfStrategy>
{
public:
  explicit
  AsfStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

public: // triggers
  void
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  beforeSatisfyInterest(const Data& data, const FaceEndpoint& ingress,
                        const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry) override;

private:
  pit::OutRecord*
  forwardInterest(const Interest& interest, Face& outFace, const fib::Entry& fibEntry,
                  const shared_ptr<pit::Entry>& pitEntry);

  void
  sendProbe(const Interest& interest, const FaceEndpoint& ingress, const Face& faceToUse,
            const fib::Entry& fibEntry, const shared_ptr<pit::Entry>& pitEntry);

  Face*
  getBestFaceForForwarding(const Interest& interest, const Face& inFace,
                           const fib::Entry& fibEntry, const shared_ptr<pit::Entry>& pitEntry,
                           bool isNewInterest = true);

  void
  onTimeoutOrNack(const Name& interestName, FaceId faceId, bool isNack);

  void
  sendNoRouteNack(Face& face, const shared_ptr<pit::Entry>& pitEntry);

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  struct FaceStatsForwardingCompare
  {
    bool
    operator()(const FaceStats& lhs, const FaceStats& rhs) const noexcept;
  };
  using FaceStatsForwardingSet = std::set<FaceStats, FaceStatsForwardingCompare>;

  AsfMeasurements m_measurements{getMeasurements()};
  std::unique_ptr<RetxSuppressionExponential> m_retxSuppression;
  ProbingModule m_probing{m_measurements};
  size_t m_nMaxTimeouts = 3;

  friend ProcessNackTraits<AsfStrategy>;
};

} // namespace asf

using asf::AsfStrategy;

} // namespace nfd::fw

#endif // NFD_DAEMON_FW_ASF_STRATEGY_HPP
