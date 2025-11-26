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

#ifndef NFD_DAEMON_RIB_READVERTISE_READVERTISE_HPP
#define NFD_DAEMON_RIB_READVERTISE_READVERTISE_HPP

#include "readvertise-destination.hpp"
#include "readvertise-policy.hpp"
#include "readvertised-route.hpp"
#include "rib/rib.hpp"

/*
 * このファイルは Readvertise クラスの定義を含み、
 * RIB（Routing Information Base）に登録された経路のうち、
 * ポリシーに合致するものを選択して別のデスティネーションへ
 * 「再広告（Readvertise）」する仕組みを実装する。
 *
 * ◆Readvertise クラスの役割
 * - RIB を監視し、経路追加・削除イベントを受け取る
 * - ReadvertisePolicy に照会して「再広告すべきか」を判断
 * - ReadvertiseDestination を通じて実際に再広告コマンドを送信
 * - 再広告済みのルートを追跡・管理する
 *
 * ◆主な構成要素
 * - m_policy:
 *     再広告を行うかどうかを決めるポリシー（例：ホスト→ゲートウェイ等）
 *
 * - m_destination:
 *     再広告を送信する宛先（例：NFD RIB管理プロトコル、別プロセスのルーティングデーモン）
 *
 * - m_rrs（ReadvertisedRouteContainer）:
 *     再広告済みルートのコンテナ。prefix や cost、署名者情報などを保持。
 *
 * - m_routeToRr（map）:
 *     RIBの経路→再広告済みルート（m_rrs）の対応表。
 *     経路削除時に正しい readvertised route を撤回するために使用。
 *
 * - m_addRouteConn / m_removeRouteConn:
 *     RIB の経路追加・削除シグナルに対する接続。
 *
 * ◆主要メソッド
 * 1. afterAddRoute()
 *    - RIB に新規経路が追加されたときに呼ばれる
 *    - ポリシーに照会し、再広告すべきなら advertise() を実行
 *
 * 2. beforeRemoveRoute()
 *    - RIB から経路が削除される前に呼ばれる
 *    - 対応する再広告済みルートを withdraw() で撤回
 *
 * 3. afterDestinationAvailable() / afterDestinationUnavailable()
 *    - デスティネーション（NFD RIB など）が利用可能/不可になった時の処理
 *    - 利用可能になったら未送信ルートを再送するなどの調整を行う
 *
 * 4. advertise()
 *    - ReadvertiseDestination へ実際の "advertise" コマンドを送信
 *
 * 5. withdraw()
 *    - ReadvertiseDestination へ "withdraw" コマンドを送信
 *
 * ◆まとめ
 * Readvertise は「RIB → ポリシー判定 → 適切な宛先へ再広告」という
 * 一連の処理を統括する中心的クラスであり、NDN の動的経路再広告機構の
 * コアロジックを構成する。
 */

namespace nfd::rib {

/** \brief Readvertise a subset of routes to a destination according to a policy.
 *
 *  The Readvertise class allows RIB routes to be readvertised to a destination such as a routing
 *  protocol daemon or another NFD-RIB. It monitors the RIB for route additions and removals,
 *  asks the ReadvertisePolicy to make decision on whether to readvertise each new route and what
 *  prefix to readvertise as, and invokes a ReadvertiseDestination to send the commands.
 */
class Readvertise : noncopyable
{
public:
  Readvertise(Rib& rib,
              unique_ptr<ReadvertisePolicy> policy,
              unique_ptr<ReadvertiseDestination> destination);

private:
  void
  afterAddRoute(const RibRouteRef& ribRoute);

  void
  beforeRemoveRoute(const RibRouteRef& ribRoute);

  void
  afterDestinationAvailable();

  void
  afterDestinationUnavailable();

  void
  advertise(ReadvertisedRouteContainer::iterator rrIt);

  void
  withdraw(ReadvertisedRouteContainer::iterator rrIt);

private:
  unique_ptr<ReadvertisePolicy> m_policy;
  unique_ptr<ReadvertiseDestination> m_destination;

  ReadvertisedRouteContainer m_rrs;
  /**
   * \brief Map from RIB route to readvertised route derived from RIB route(s).
   */
  std::map<RibRouteRef, ReadvertisedRouteContainer::iterator> m_routeToRr;

  signal::ScopedConnection m_addRouteConn;
  signal::ScopedConnection m_removeRouteConn;
};

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_READVERTISE_READVERTISE_HPP
