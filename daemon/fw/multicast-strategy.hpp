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

#ifndef NFD_DAEMON_FW_MULTICAST_STRATEGY_HPP
#define NFD_DAEMON_FW_MULTICAST_STRATEGY_HPP

#include "strategy.hpp"
#include "retx-suppression-exponential.hpp"

/* -------------------------------------------------------------------------
 * MulticastStrategy.hpp の解説
 *
 * ■ 概要
 *   - FIB（Forwarding Information Base）の全ての次ホップに対して
 *     Interest を転送する戦略。
 *   - 一般的に、目的のデータが複数経路に存在する場合や
 *     ループ回避・探索のために利用される。
 *
 * ■ クラス構造
 *
 * MulticastStrategy : public Strategy
 *   - コンストラクタで Forwarder と戦略名を受け取る
 *   - static getStrategyName() で戦略名を返す
 *
 * トリガーメソッド:
 *   - afterReceiveInterest()
 *       -> PIT に対応する Interest を受信した際に呼ばれる
 *       -> 全 FIB nexthop に Interest を転送
 *
 *   - onInterestLoop()
 *       -> Interest がループした場合に呼ばれる
 *       -> この戦略では何もしない
 *
 *   - afterNewNextHop()
 *       -> PIT エントリに新しい nexthop が追加された場合に呼ばれる
 *
 * メンバ:
 *   - std::unique_ptr<RetxSuppressionExponential> m_retxSuppression
 *       -> 再送抑制アルゴリズム（指数バックオフ）を管理
 *
 * ■ 特徴
 *   - FIB の全 nexthop に Interest を送信するため、データ探索に強い
 *   - 再送抑制により無駄な再送を軽減
 *   - Interest ループ時は無処理（無限ループ防止は別機構で対応）
 *
 * ■ まとめ
 *   - 名前空間ベースで全経路に Interest を投げる探索的戦略
 *   - 再送抑制と組み合わせることでネットワーク負荷を抑制
 * ------------------------------------------------------------------------- */

namespace nfd::fw {

/**
 * \brief A forwarding strategy that forwards Interests to all FIB nexthops.
 */
class MulticastStrategy : public Strategy
{
public:
  explicit
  MulticastStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

public: // triggers
  void
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  onInterestLoop(const Interest& interest, const FaceEndpoint& ingress) override
  {
    // do nothing
  }

  void
  afterNewNextHop(const fib::NextHop& nextHop, const shared_ptr<pit::Entry>& pitEntry) override;

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  std::unique_ptr<RetxSuppressionExponential> m_retxSuppression;
};

} // namespace nfd::fw

#endif // NFD_DAEMON_FW_MULTICAST_STRATEGY_HPP
