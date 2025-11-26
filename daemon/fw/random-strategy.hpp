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

#ifndef NFD_DAEMON_FW_RANDOM_STRATEGY_HPP
#define NFD_DAEMON_FW_RANDOM_STRATEGY_HPP

#include "strategy.hpp"
#include "process-nack-traits.hpp"

/* -------------------------------------------------------------------------
 * RandomStrategy.hpp の解説
 *
 * ■ 概要
 *   NFD における転送戦略の一つ。
 *   受信した Interest を、受信フェース以外のアウトゴーイングフェースの中から
 *   ランダムに選んで転送する。
 *
 * ■ クラス: RandomStrategy
 *
 * 継承:
 *   - Strategy: NFD の基本戦略クラス
 *   - ProcessNackTraits<RandomStrategy>: NACK 処理の共通機能を提供
 *
 * public:
 *   - RandomStrategy(Forwarder& forwarder, const Name& name = getStrategyName())
 *       - コンストラクタ
 *
 *   - static const Name& getStrategyName()
 *       - 戦略名を返す
 *
 * public triggers:
 *   - void afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
 *                               const shared_ptr<pit::Entry>& pitEntry)
 *       - Interest 受信時の処理
 *
 *   - void afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
 *                           const shared_ptr<pit::Entry>& pitEntry)
 *       - NACK 受信時の処理
 *
 * private:
 *   - ProcessNackTraits<RandomStrategy> が友達クラスとして NACK 処理を補助
 *
 * ■ 動作イメージ
 *   - PIT に Interest を記録
 *   - 受信フェース以外の利用可能なフェースをリストアップ
 *   - その中からランダムに 1 つ選んで Interest を送信
 *   - NACK が返ってきた場合も ProcessNackTraits が適切に処理
 *
 * ■ まとめ
 *   - ネットワークパスが複数存在する場合、負荷分散効果がある
 *   - シンプルで理解しやすいが、最適経路選択は行わない
 * ------------------------------------------------------------------------- */

namespace nfd::fw {

/**
 * \brief A forwarding strategy that randomly chooses a nexthop.
 *
 * Sends the incoming Interest to a random outgoing face, excluding the incoming face.
 */
class RandomStrategy : public Strategy
                     , public ProcessNackTraits<RandomStrategy>
{
public:
  explicit
  RandomStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

public: // triggers
  void
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry) override;

private:
  friend ProcessNackTraits<RandomStrategy>;
};

} // namespace nfd::fw

#endif // NFD_DAEMON_FW_RANDOM_STRATEGY_HPP
