/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#ifndef NFD_DAEMON_RIB_READVERTISE_READVERTISED_ROUTE_HPP
#define NFD_DAEMON_RIB_READVERTISE_READVERTISED_ROUTE_HPP

#include "core/common.hpp"

#include <ndn-cxx/security/signing-info.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <set>

/*
 * このファイルは ReadvertisedRoute クラスおよびそのコンテナ定義を含み、
 * 「再広告（Readvertise）された経路の状態」を保持・管理するための
 * データ構造を提供する。
 *
 * ◆ReadvertisedRoute クラスの役割
 * - Policy により再広告対象と判断された経路を記録するための状態オブジェクト
 * - prefix や cost、署名情報、関連する RIB 経路数など、
 *   再広告に必要なメタデータをまとめて保持する
 * - Readvertise クラス（上位管理クラス）から利用される
 *
 * ◆主要フィールド
 * - prefix:
 *     再広告する Name Prefix。
 *
 * - cost:
 *     当該プレフィックス到達に必要なコスト。
 *     複数 RIB 経路から構築された場合でも代表コストを保持。
 *
 * - signer (mutable):
 *     コマンド送信時の署名に使用する SigningInfo。
 *     個々の ReadvertiseDestination により設定される。
 *
 * - nRibRoutes (mutable):
 *     このプレフィックスの再広告を引き起こした RIB Route の数。
 *     RIB に複数経路が存在し、それらが同じ prefix にマップされる場合に増える。
 *     RIB Route が削除されて 0 になれば再広告を撤回してよい。
 *
 * - retryDelay / retryEvt (mutable):
 *     再広告や再試行を行うためのスケジューラ管理情報。
 *     エラー発生時の再送や refresh 処理を行うために利用。
 *
 * ◆比較演算子
 * - operator<()
 *     std::set に格納するため、prefix の辞書順で比較。
 *     これにより ReadvertisedRouteContainer（std::set）が安定して管理できる。
 *
 * ◆ReadvertisedRouteContainer
 * - using ReadvertisedRouteContainer = std::set<ReadvertisedRoute>;
 *     再広告済みルートをセット構造で保持。
 *     prefix をキーとした高速探索・ソートが可能。
 *
 * ◆まとめ
 * ReadvertisedRoute は「どの prefix をどの署名で、何件の RIB 経路から
 * 形成されたか」を保持する再広告状態管理の基本データ構造。
 * Readvertise クラスの内部で、再広告の登録・撤回・再試行スケジューリングを
 * 適切に行うための中核モデルとなる。
 */

namespace nfd::rib {

/**
 * \brief State of a readvertised route.
 */
class ReadvertisedRoute : noncopyable
{
public:
  ReadvertisedRoute(const Name& prefix, uint64_t cost)
    : prefix(prefix)
    , cost(cost)
  {
  }

  friend bool
  operator<(const ReadvertisedRoute& lhs, const ReadvertisedRoute& rhs)
  {
    return lhs.prefix < rhs.prefix;
  }

public:
  Name prefix; ///< readvertised prefix
  uint64_t cost; ///< cost to reach the prefix
  mutable ndn::security::SigningInfo signer; ///< signer for commands
  mutable size_t nRibRoutes = 0; ///< number of RIB routes that cause the readvertisement
  mutable time::milliseconds retryDelay = 0_ms; ///< retry interval (not used for refresh)
  mutable ndn::scheduler::ScopedEventId retryEvt; ///< retry or refresh event
};

using ReadvertisedRouteContainer = std::set<ReadvertisedRoute>;

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_READVERTISE_READVERTISED_ROUTE_HPP
