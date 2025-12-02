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

#ifndef NFD_DAEMON_RIB_ROUTE_HPP
#define NFD_DAEMON_RIB_ROUTE_HPP

#include "core/common.hpp"

#include <ndn-cxx/encoding/nfd-constants.hpp>
#include <ndn-cxx/mgmt/nfd/route-flags-traits.hpp>
#include <ndn-cxx/prefix-announcement.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <type_traits>

/*
 * === File Overview: Route (RIB Route Representation) ================================
 *
 * このファイルは NFD (Named Data Networking Forwarding Daemon) の
 * Routing Information Base (RIB) 内で利用される **Route クラス** を定義する。
 *
 * Route は「ある名前プレフィックスに対してどの Face へ転送するのか」という
 * ルーティング情報の最小単位であり、次の情報を保持する。
 *
 * -----------------------------------------------------------------------------
 * 【Route クラスの役割】
 *   - RIB に登録される「1つの経路情報（route）」を表現するデータ構造
 *   - ルートの属性（faceId, cost, origin, flags など）を保持
 *   - PrefixAnnouncement から生成されるルートに対応
 *   - 有効期限付きルート（expires, annExpires）を管理
 *   - スケジューラを使ったルート有効期限イベント m_expirationEvent の管理
 *
 * -----------------------------------------------------------------------------
 * 【主なメンバ変数】
 *   - faceId:
 *       このルートが向く Face の識別子
 *   - origin:
 *       ルートの起源種別（アプリケーション、管理コマンド、PrefixAnnouncement 等）
 *   - cost:
 *       ルートのコスト値（PrefixAnnouncement の場合は PA_ROUTE_COST=2048 ）
 *   - flags:
 *       ndn::nfd::RouteFlags に基づくフラグビット
 *   - expires:
 *       Route の有効期限（Route 全体に対する expiration）
 *   - announcement:
 *       PrefixAnnouncement から生成された場合のメタ情報
 *   - annExpires:
 *       PrefixAnnouncement 自体の有効期限タイムスタンプ
 *   - m_expirationEvent:
 *       scheduler による expiration イベント ID
 *
 * -----------------------------------------------------------------------------
 * 【コンストラクタ】
 *   - Route():
 *       デフォルト初期化
 *   - Route(const PrefixAnnouncement&, faceId):
 *       PrefixAnnouncement を元に Route を生成
 *       主に動的なプレフィックス広告を受信した際に使用される
 *
 * -----------------------------------------------------------------------------
 * 【主なメソッド】
 *   - getExpirationEvent():
 *       有効期限イベント ID を返す
 *   - setExpirationEvent():
 *       有効期限イベント ID をセット
 *   - cancelExpirationEvent():
 *       期限切れイベントのキャンセル
 *   - getFlags():
 *       RouteFlags を数値として取得
 *
 * -----------------------------------------------------------------------------
 * 【演算子】
 *   - operator==:
 *       faceId・origin・cost・flags・expires・announcement を比較し
 *       Route の等価性を判断
 *
 * -----------------------------------------------------------------------------
 * 【総合】
 *   Route クラスは RIB の「最小単位のルート情報」を表すクラスであり、
 *   以下を主眼として設計されている。
 *     - NDN の PrefixAnnouncement や管理コマンドによるルート追加へ対応
 *     - ルートの種類・出自・有効期限を一元的に管理
 *     - RIB からの削除タイミングを scheduler と連携して制御
 *
 *   RIB (RibEntry / Rib) の上位構造と連携し、NDN の階層型名前空間に基づく
 *   安全かつ動的なルーティング管理を支える基盤となる。
 *
 * ================================================================================
 */

namespace nfd::rib {

/**
 * \brief Represents a route for a name prefix.
 */
class Route : public ndn::nfd::RouteFlagsTraits<Route>, private boost::equality_comparable<Route>
{
public:
  /** \brief Default constructor.
   */
  Route();

  /** \brief Construct from a prefix announcement.
   *  \param ann a prefix announcement that has passed verification
   *  \param faceId the face on which \p ann arrived
   */
  Route(const ndn::PrefixAnnouncement& ann, uint64_t faceId);

  const ndn::scheduler::EventId&
  getExpirationEvent() const
  {
    return m_expirationEvent;
  }

  void
  setExpirationEvent(ndn::scheduler::EventId&& eid)
  {
    m_expirationEvent = std::move(eid);
  }

  void
  cancelExpirationEvent()
  {
    m_expirationEvent.cancel();
  }

  std::underlying_type_t<ndn::nfd::RouteFlags>
  getFlags() const
  {
    return flags;
  }

public: // non-member operators (hidden friends)
  friend bool
  operator==(const Route& lhs, const Route& rhs)
  {
    return lhs.faceId == rhs.faceId &&
           lhs.origin == rhs.origin &&
           lhs.cost == rhs.cost &&
           lhs.flags == rhs.flags &&
           lhs.expires == rhs.expires &&
           lhs.announcement == rhs.announcement;
  }

public:
  /// Cost of route created by prefix announcement.
  static constexpr uint64_t PA_ROUTE_COST = 2048;

  uint64_t faceId = 0;
  ndn::nfd::RouteOrigin origin = ndn::nfd::ROUTE_ORIGIN_APP;
  uint64_t cost = 0;
  std::underlying_type_t<ndn::nfd::RouteFlags> flags = ndn::nfd::ROUTE_FLAGS_NONE;
  std::optional<time::steady_clock::time_point> expires;

  /** \brief The prefix announcement that caused the creation of this route.
   *
   *  This is nullopt if this route is not created by a prefix announcement.
   */
  std::optional<ndn::PrefixAnnouncement> announcement;

  /** \brief Expiration time of the prefix announcement.
   *
   *  Valid only if announcement is not nullopt.
   *
   *  If this field is before or equal the current time, it indicates the prefix announcement is
   *  not yet valid or has expired. In this case, the exact value of this field does not matter.
   *  If this field is after the current time, it indicates when the prefix announcement expires.
   */
  time::steady_clock::time_point annExpires;

private:
  ndn::scheduler::EventId m_expirationEvent;
};

std::ostream&
operator<<(std::ostream& os, const Route& route);

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_ROUTE_HPP
