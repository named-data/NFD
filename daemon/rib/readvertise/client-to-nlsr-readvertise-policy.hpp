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

#ifndef NFD_DAEMON_RIB_READVERTISE_CLIENT_TO_NLSR_READVERTISE_POLICY_HPP
#define NFD_DAEMON_RIB_READVERTISE_CLIENT_TO_NLSR_READVERTISE_POLICY_HPP

#include "readvertise-policy.hpp"

/*
 * このファイルは「ClientToNlsrReadvertisePolicy」クラスを定義し、
 * クライアント（エンドホスト）によって登録された RIB 経路を
 * NLSR（Named-data Link State Routing）へ再広告するかどうかを
 * 判定するためのポリシーを提供する。
 *
 * ◆役割概要
 * - ReadvertisePolicy の具体実装。
 * - RIB に新しい経路が追加された際、
 *   その経路が「クライアント起源（ROUTE_ORIGIN_CLIENT）」であれば
 *   NLSR へ再広告すべきだと判断する。
 * - 再広告時にはデフォルト署名情報（default signing identity）を利用する。
 *
 * ◆handleNewRoute()
 * - RIB 経路（RibRouteRef）を受け取り、
 *   その origin が client かどうかをチェックする。
 * - client であれば ReadvertiseAction を返す。
 *     - prefix: 再広告すべきプレフィックス
 *     - cost: 経路コスト（RIB 由来の値）
 *     - signer: NLSR に送信するコマンド署名に使うデフォルト SigningInfo
 * - それ以外（ルータ由来など）は std::nullopt を返し、再広告しない。
 *
 * ◆getRefreshInterval()
 * - 再広告のリフレッシュ（再送）間隔を返す。
 * - NLSR 向けの再広告には一定の有効期間と更新タイミングが必要なため、
 *   Readvertise クラス側でこの値を使って periodic refresh を行う。
 *
 * ◆利用される場面
 * - Readvertise クラスが RIB の route addition イベントを受け取った際、
 *   このポリシーを用いて「再広告すべきかどうか」を判定する。
 * - NLSR を使用するネットワークで、
 *   クライアントが自動的に prefix を広告できる仕組みを実現する一部。
 *
 * ◆まとめ
 * ClientToNlsrReadvertisePolicy は、
 * 「クライアントが登録した prefix を NLSR の LSDB へ反映させる」
 * ための最小限の判定ロジックを提供する軽量な再広告ポリシーである。
 */

namespace nfd::rib {

/** \brief A policy to readvertise routes registered by end hosts into NLSR.
 */
class ClientToNlsrReadvertisePolicy : public ReadvertisePolicy
{
public:
  /** \brief Advertise if the route's origin is client.
   *
   *  If the route origin is "client" (typically from auto prefix propagation), readvertise it
   *  using the default signing identity.
   */
  std::optional<ReadvertiseAction>
  handleNewRoute(const RibRouteRef& ribRoute) const override;

  time::milliseconds
  getRefreshInterval() const override;
};

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_READVERTISE_CLIENT_TO_NLSR_READVERTISE_POLICY_HPP
