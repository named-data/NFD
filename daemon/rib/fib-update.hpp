/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_DAEMON_RIB_FIB_UPDATE_HPP
#define NFD_DAEMON_RIB_FIB_UPDATE_HPP

#include "core/common.hpp"

#include <boost/operators.hpp>

/*
 * 【概要】
 * このヘッダファイルは、NFD (Named Data Networking Forwarding Daemon) の
 * RIB（Routing Information Base）から FIB（Forwarding Information Base）へ
 * 伝達される更新情報を表現するためのクラス **FibUpdate** を定義している。
 *
 * 【FibUpdate クラスの役割】
 * - RIB でルート情報が変更された際に、FIB に対して「次ホップの追加」または
 *   「次ホップの削除」を指示するデータ構造として機能する。
 * - FIB 更新の種類（ADD / REMOVE）、対象となる名前、faceId、コストなどの
 *   必要情報を保持する。
 *
 * 【主要フィールド】
 * - `Name name`  
 *     → 更新対象となる名前プレフィックス
 *
 * - `uint64_t faceId`  
 *     → 次ホップに対応する Face の ID  
 *       （ADD / REMOVE いずれの操作でも使用）
 *
 * - `uint64_t cost`  
 *     → 経路コスト（ADD の場合のみ意味を持つ）
 *
 * - `Action action`  
 *     → ADD_NEXTHOP または REMOVE_NEXTHOP を示す列挙型
 *
 * 【主な機能】
 * ● `createAddUpdate(name, faceId, cost)`  
 *     → 次ホップ追加用の FibUpdate を生成
 *
 * ● `createRemoveUpdate(name, faceId)`  
 *     → 次ホップ削除用の FibUpdate を生成
 *
 * ● `operator==`  
 *     → 2 つの FibUpdate が完全に一致するか比較できる
 *
 * ● `operator<<`  
 *     → FibUpdate の情報を文字列として出力（ログで利用）
 *
 * 【用途】
 * - RIB モジュールが、ルーティング変更を Forwarder の FIB へ反映させる際に利用。
 * - FIB の更新処理は Forwarder のパケット転送に影響するため、重要な中間情報。
 *
 * このファイルは、RIB と FIB の間の「更新通知」のための最小限のデータ構造を提供する。
 */

namespace nfd::rib {

/**
 * \brief Represents a FIB update.
 */
class FibUpdate : private boost::equality_comparable<FibUpdate>
{
public:
  enum Action {
    ADD_NEXTHOP    = 0,
    REMOVE_NEXTHOP = 1,
  };

  static FibUpdate
  createAddUpdate(const Name& name, uint64_t faceId, uint64_t cost);

  static FibUpdate
  createRemoveUpdate(const Name& name, uint64_t faceId);

public: // non-member operators (hidden friends)
  friend bool
  operator==(const FibUpdate& lhs, const FibUpdate& rhs) noexcept
  {
    return lhs.name == rhs.name &&
           lhs.faceId == rhs.faceId &&
           lhs.cost == rhs.cost &&
           lhs.action == rhs.action;
  }

public:
  Name name;
  uint64_t faceId = 0;
  uint64_t cost = 0;
  Action action;
};

std::ostream&
operator<<(std::ostream& os, const FibUpdate& update);

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_FIB_UPDATE_HPP
