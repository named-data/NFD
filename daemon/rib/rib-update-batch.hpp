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

#ifndef NFD_DAEMON_RIB_RIB_UPDATE_BATCH_HPP
#define NFD_DAEMON_RIB_RIB_UPDATE_BATCH_HPP

#include "route.hpp"

#include <list>

/*
 * === File Overview: RibUpdate & RibUpdateBatch ====================================
 *
 * このファイルは NFD（Named Data Networking Forwarding Daemon）の RIB
 * （Routing Information Base）に対する更新情報を表現するための
 * データ構造を定義している。RIB へ適用される「ルート追加・削除」などの
 * 更新操作を表し、それらを FaceId ごとにまとめて扱うためのクラス群を提供する。
 *
 * 【RibUpdate の役割】
 * - 1つのルート更新操作（add/remove 等）を表す単位データ構造
 * - 内容：
 *     ・action: 更新種別（REGISTER / UNREGISTER / REMOVE_FACE）
 *     ・name  : 更新対象となる名前プレフィックス
 *     ・route : 追加・削除対象の Route 情報
 *
 * 【Action の意味】
 * - REGISTER   : route を登録（追加）する更新
 * - UNREGISTER : route を登録解除（削除）する更新
 * - REMOVE_FACE: Face 破棄通知により、その face に紐づく route を削除する更新
 *
 * 【補助機能】
 * - operator<< : RibUpdate, Action を人間が読める文字列形式へ出力
 *
 * 【RibUpdateBatch の役割】
 * - 特定の FaceId に対して適用される複数の RibUpdate をまとめて保持するクラス
 * - “face 単位での RIB 更新バッチ処理” を実現するためのデータ構造
 *
 *  主な機能：
 *   - getFaceId(): このバッチが対象とする FaceId を取得
 *   - add(): RibUpdate をバッチへ追加
 *   - begin(), end(): 追加された更新一覧を iterator として取得
 *   - size(): バッチ内の更新数を取得
 *
 * 【内部状態】
 *   - m_faceId : この更新バッチが対象とする FaceId
 *   - m_updates: RibUpdate の list（更新操作の集合）
 *
 * このファイルは FIB/RIB Updater などの上位コンポーネントから利用され、
 * 「どの face に対して、どんな更新をまとめて適用するか」を管理するための
 * 重要な構造を提供する。
 *
 * ================================================================================
 */

namespace nfd::rib {

/**
 * \brief Represents a route that will be added to or removed from a namespace.
 */
struct RibUpdate
{
  enum Action {
    REGISTER    = 0,
    UNREGISTER  = 1,
    /**
     * \brief An update triggered by a face destruction notification
     * \note indicates a Route needs to be removed after a face is destroyed
     */
    REMOVE_FACE = 2,
  };

  Action action;
  Name name;
  Route route;
};

std::ostream&
operator<<(std::ostream& os, RibUpdate::Action action);

std::ostream&
operator<<(std::ostream& os, const RibUpdate& update);

using RibUpdateList = std::list<RibUpdate>;

/**
 * \brief Represents a collection of RibUpdates to be applied to a single FaceId.
 */
class RibUpdateBatch
{
public:
  using const_iterator = RibUpdateList::const_iterator;

  explicit
  RibUpdateBatch(uint64_t faceId);

  uint64_t
  getFaceId() const noexcept
  {
    return m_faceId;
  }

  void
  add(const RibUpdate& update);

  const_iterator
  begin() const noexcept
  {
    return m_updates.begin();
  }

  const_iterator
  end() const noexcept
  {
    return m_updates.end();
  }

  size_t
  size() const noexcept
  {
    return m_updates.size();
  }

private:
  uint64_t m_faceId;
  RibUpdateList m_updates;
};

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_RIB_UPDATE_BATCH_HPP
