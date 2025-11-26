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

#ifndef NFD_DAEMON_RIB_READVERTISE_HOST_TO_GATEWAY_READVERTISE_POLICY_HPP
#define NFD_DAEMON_RIB_READVERTISE_HOST_TO_GATEWAY_READVERTISE_POLICY_HPP

#include "readvertise-policy.hpp"
#include "common/config-file.hpp"

#include <ndn-cxx/security/key-chain.hpp>

/*
 * HostToGatewayReadvertisePolicy.hpp
 *
 * 概要:
 *   NFD の RIB (Routing Information Base) における Readvertise 機能の一種で、
 *   「ローカルアプリケーションが登録したルート情報を
 *     リモート側のゲートウェイへ再広告する（readvertise）」
 *   ためのポリシーを提供するクラス。
 *
 * 役割:
 *   - RIB に新規ルートが追加された際に、再広告するべきか判定
 *   - Readvertise の更新周期（refresh interval）を提供
 *   - KeyChain を利用したセキュリティ設定を保持
 *
 * 主な機能:
 *   - handleNewRoute():
 *        RIB に新たなルートが登録された際に呼ばれ、
 *        再広告するための ReadvertiseAction を返す
 *        → 不要な場合は std::nullopt
 *
 *   - getRefreshInterval():
 *        再広告を定期的に実施するための間隔（秒指定）を返す
 *
 * 設定:
 *   - ConfigSection より refresh interval 等を読み取る設計
 *   - KeyChain により署名等の必要なセキュリティ処理を可能にする
 *
 * 位置付け:
 *   - ReadvertisePolicy の派生クラス
 *   - ホスト内で登録された名前空間を、ゲートウェイや他のネットワーク領域へ伝搬し、
 *     名前解決・データルーティングの到達性向上を目的とするポリシー
 */

namespace nfd::rib {

/** \brief A policy to readvertise routes registered by local applications into remote gateway.
 */
class HostToGatewayReadvertisePolicy : public ReadvertisePolicy
{
public:
  HostToGatewayReadvertisePolicy(const ndn::KeyChain& keyChain,
                                 const ConfigSection& section);

public:
  std::optional<ReadvertiseAction>
  handleNewRoute(const RibRouteRef& ribRoute) const override;

  time::milliseconds
  getRefreshInterval() const override;

private:
  const ndn::KeyChain& m_keyChain;
  time::seconds m_refreshInterval;
};

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_READVERTISE_HOST_TO_GATEWAY_READVERTISE_POLICY_HPP
