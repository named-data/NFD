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

#ifndef NFD_DAEMON_RIB_READVERTISE_READVERTISE_POLICY_HPP
#define NFD_DAEMON_RIB_READVERTISE_READVERTISE_POLICY_HPP

#include "rib/rib.hpp"

#include <ndn-cxx/security/signing-info.hpp>

/*
 * このファイルは ReadvertisePolicy クラスおよび ReadvertiseAction 構造体の
 * 定義を含む。ReadvertisePolicy は、RIB に登録された経路をどのように
 * 再広告（Readvertise）するかを決定するためのポリシーを抽象化した基底クラスである。
 *
 * ◆ReadvertiseAction（再広告の決定内容を保持）
 * - prefix:
 *     再広告すべき名前プレフィックス
 * - cost:
 *     経路コスト（再広告先に伝えるメトリックとして利用）
 * - signer:
 *     再広告コマンド署名に使用する認証情報（SigningInfo）
 *
 * ◆ReadvertisePolicy の役割
 * - 新しい経路が RIB に登録された際、
 *   その経路を再広告するべきかどうか判定し、
 *   必要であれば ReadvertiseAction を生成する
 * - 再広告した経路を定期リフレッシュする間隔も提供する
 *
 * ◆主要機能（純粋仮想関数）
 * 1. handleNewRoute()
 *    - 引数：RIB に新規登録された経路
 *    - 戻り値：再広告すべき場合は ReadvertiseAction、不要なら std::nullopt
 *
 * 2. getRefreshInterval()
 *    - 再広告済み経路の更新（再通知）間隔を返す
 *
 * ◆まとめ
 * 本クラスは「どの経路をどの条件で再広告するか」を決める
 * 制御ロジックを抽象化したもの。
 * 具体的なポリシーは派生クラス（例：HostToGatewayReadvertisePolicy）
 * にて実装し、個々の利用環境に応じた再広告戦略を適用可能とする。
 */

namespace nfd::rib {

/** \brief A decision made by readvertise policy.
 */
struct ReadvertiseAction
{
  Name prefix; ///< the prefix that should be readvertised
  uint64_t cost; ///< route cost
  ndn::security::SigningInfo signer; ///< credentials for command signing
};

/** \brief A policy to decide whether to readvertise a route, and what prefix to readvertise.
 */
class ReadvertisePolicy : noncopyable
{
public:
  virtual
  ~ReadvertisePolicy() = default;

  /** \brief Decide whether to readvertise a route, and what prefix to readvertise.
   */
  virtual std::optional<ReadvertiseAction>
  handleNewRoute(const RibRouteRef& ribRoute) const = 0;

  /** \return how often readvertisements made by this policy should be refreshed.
   */
  virtual time::milliseconds
  getRefreshInterval() const = 0;
};

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_READVERTISE_READVERTISE_POLICY_HPP
