/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

/* -------------------------------------------------------------------------
 * BestRouteStrategy.hpp の解説
 *
 * ■ 概要
 *   - 新しい Interest を最もコストの低い nexthop に転送する戦略。
 *   - 再送が発生した場合（指数バックオフで抑制されない場合）、まだ使われていない
 *     最も低コストの nexthop に転送。
 *   - すべての nexthop を使い切った場合は、再度最初の nexthop から開始。
 *   - FIB に nexthop がない場合や、スコープ違反の場合、Nack を返す。
 *   - すべての upstream から Nack を受けた場合も、最も軽度な理由で Nack を返す。
 *
 * ■ クラス構造
 *
 * BestRouteStrategy : public Strategy, ProcessNackTraits<BestRouteStrategy>
 *   - コンストラクタで Forwarder と戦略名を受け取る
 *   - static getStrategyName() で戦略名を返す
 *
 * トリガーメソッド:
 *   - afterReceiveInterest()
 *       -> PIT に対応する Interest を受信した際に呼ばれる
 *       -> 最良経路に基づき Interest を転送
 *
 *   - afterReceiveNack()
 *       -> upstream から Nack を受信した際に呼ばれる
 *       -> ProcessNackTraits を利用して downstream に Nack を返す
 *
 * メンバ:
 *   - std::unique_ptr<RetxSuppressionExponential> m_retxSuppression
 *       -> 再送抑制アルゴリズム（指数バックオフ）を管理
 *
 * ■ 特徴
 *   - 最良経路に基づき Interest を転送することで効率的なデータ取得を実現
 *   - 再送抑制により無駄な再送を軽減
 *   - Nack 処理は ProcessNackTraits による共通処理を利用
 *
 * ■ まとめ
 *   - 名前空間ベースで最適経路を選択する戦略
 *   - 再送抑制と Nack 共通処理を組み合わせることで効率的かつ安全に転送
 * ------------------------------------------------------------------------- */

namespace nfd::fw {

/**
 * \brief "Best route" forwarding strategy.
 *
 * This strategy forwards a new Interest to the lowest-cost nexthop (except downstream).
 * After that, if consumer retransmits the Interest (and is not suppressed according to
 * exponential backoff algorithm), the strategy forwards the Interest again to
 * the lowest-cost nexthop (except downstream) that is not previously used.
 * If all nexthops have been used, the strategy starts over with the first nexthop.
 *
 * This strategy returns Nack to all downstreams with reason NoRoute
 * if there is no usable nexthop, which may be caused by:
 *  (a) the FIB entry contains no nexthop;
 *  (b) the FIB nexthop happens to be the sole downstream;
 *  (c) the FIB nexthops violate scope.
 *
 * This strategy returns Nack to all downstreams if all upstreams have returned Nacks.
 * The reason of the sent Nack equals the least severe reason among received Nacks.
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
  std::unique_ptr<RetxSuppressionExponential> m_retxSuppression;

  friend ProcessNackTraits<BestRouteStrategy>;
};

} // namespace nfd::fw

#endif // NFD_DAEMON_FW_BEST_ROUTE_STRATEGY_HPP
