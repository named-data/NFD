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

#ifndef NFD_DAEMON_RIB_RIB_HPP
#define NFD_DAEMON_RIB_RIB_HPP

#include "rib-entry.hpp"
#include "rib-update-batch.hpp"

#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>

#include <functional>
#include <map>

/*
 * === File Overview: Rib (Routing Information Base) =================================
 *
 * このファイルは NFD (Named Data Networking Forwarding Daemon) における
 * Routing Information Base（RIB）の中心的なデータ構造を定義する。
 * RIB は NDN の名前空間に対して登録された Route の集合を管理し、
 * 各名前プレフィックスに対応する RibEntry を保持する階層構造の
 * ルーティングデータベースである。
 *
 * 本ファイルは以下の役割を担う：
 *   - RIB 全体を表すクラス Rib の定義
 *   - RIB 内の単一 Route を参照する RibRouteRef の定義
 *   - FIB との同期処理（FibUpdater）との連携
 *   - ルート登録/削除/Face 破棄に伴う RIB 更新処理の開始
 *   - 更新バッチ（RibUpdateBatch）の管理と更新キュー処理
 *   - 名前階層に基づく継承ルート（inherited routes）の取得／反映
 *   - RIB の変更を通知するシグナル発行（afterInsertEntry, afterAddRoute など）
 *
 * -----------------------------------------------------------------------------
 * 【RibRouteRef】
 *   - RIB 内の特定の Route への参照を保持する構造体
 *   - RibEntry とその entry 内の Route iterator の組で識別
 *   - operator< により、ルート比較のための順序付けが提供される
 *
 * -----------------------------------------------------------------------------
 * 【クラス Rib の概要】
 *   RIB 全体の管理を行うクラスであり、以下の責務を持つ。
 *
 *   ■ 基本データ管理
 *     - m_rib: Name → RibEntry のマップ (RIB の主要テーブル)
 *     - m_faceEntries: FaceId → この face を利用する RibEntry の対応付け
 *     - m_nItems: 登録されている Route 数
 *
 *   ■ 参照インタフェース
 *     - find(prefix):     指定 prefix の RibEntry を取得
 *     - find(prefix,route): 指定 prefix 下の一致する Route を取得
 *     - findLongestPrefix(prefix, route):
 *         longest prefix match (LPM) に基づき Route を探索
 *     - findParent(prefix): prefix の親となるエントリを探索
 *
 *   ■ 更新の開始 (FIB 更新を伴う操作)
 *     - beginApplyUpdate():
 *         RibUpdate を受け取り、FibUpdater に FIB 更新処理を依頼。
 *         成功時：RIB へ更新適用
 *         失敗時：RIB は変更せず、エラーを返す
 *     - beginRemoveFace():
 *         Face 破棄に伴う route の削除処理を開始
 *     - beginRemoveFailedFaces():
 *         使用不能になった face 群に対応する削除処理を開始
 *     - insert():
 *         新規 Route を RIB に登録（内部で更新キューへ enqueue）
 *     - onRouteExpiration():
 *         有効期限切れによる route 削除
 *
 *   ■ 更新キューとバッチ処理
 *     - RibUpdateBatch を update queue に蓄積し、FIFO で処理
 *     - sendBatchFromQueue():
 *         現在処理中の更新がなければ、キュー先頭のバッチを FIB 更新へ送信
 *     - onFibUpdateSuccess(), onFibUpdateFailure():
 *         FIB 側の結果に基づき RIB の整合性を更新 / rollback
 *
 *   ■ ルート継承（Inherited Routes）
 *     - getAncestorRoutes(entry or name):
 *         名前階層に基づく祖先プレフィックスから継承する Route を取得
 *     - modifyInheritedRoutes():
 *         継承された Route 群を各 RibEntry の inheritedRoutes に適用
 *     - findDescendants(prefix):
 *         prefix 配下にある子孫エントリを取得（削除・継承更新用途）
 *
 *   ■ RIB エントリ操作
 *     - erase() / eraseEntry():
 *         指定 route の削除、および そのプレフィックスに対応する RIB entry の
 *         route が空になった場合は Entry ごと削除
 *
 * -----------------------------------------------------------------------------
 * 【FibUpdater との連携】
 *   RIB 単独では FIB を変更しない。  
 *   全てのルーティング更新は FibUpdater を通して FIB と同期した上で最終適用される。
 *   この “FIB 更新成功後に RIB 更新を適用する” という非同期処理を管理するため、
 *   更新バッチと更新キューが不可欠な役割を担う。
 *
 * -----------------------------------------------------------------------------
 * 【シグナル（通知機構）】
 *   RIB の状態変化を外部へ通知する signal::Signal:
 *     - afterInsertEntry: 新規プレフィックスが RIB に追加された直後
 *     - afterEraseEntry : プレフィックスが RIB から削除された直後
 *     - afterAddRoute   : Route が追加された直後
 *     - beforeRemoveRoute: Route が削除される直前
 *
 *   これにより、管理ツールや統計収集などが RIB の更新を追跡可能になる。
 *
 * -----------------------------------------------------------------------------
 * 【総合】
 *   Rib クラスは NFD のルーティング管理の中核を成し、
 *   “名前空間ごとの Route 登録” と “階層的継承ルート管理”、
 *   “FIB 同期との整合性確保” の 3 つを中心に、完全な RIB 機能を提供する。
 *
 * ================================================================================
 */
namespace nfd::rib {

using ndn::nfd::ControlParameters;

class FibUpdater;

/**
 * \brief References a route.
 */
struct RibRouteRef
{
  shared_ptr<RibEntry> entry;
  RibEntry::const_iterator route;

  friend bool
  operator<(const RibRouteRef& lhs, const RibRouteRef& rhs) noexcept
  {
    return std::tie(lhs.entry->getName(), lhs.route->faceId, lhs.route->origin) <
           std::tie(rhs.entry->getName(), rhs.route->faceId, rhs.route->origin);
  }
};

/**
 * \brief Represents the Routing Information Base.
 *
 * The Routing Information Base (RIB) contains a collection of Route objects,
 * each representing a piece of static or dynamic routing information registered
 * by applications, operators, or NFD itself. Routes associated with the same
 * namespace are collected into a RIB entry.
 *
 * \sa RibEntry
 */
class Rib : noncopyable
{
public:
  using RibEntryList = std::list<shared_ptr<RibEntry>>;
  using RibTable = std::map<Name, shared_ptr<RibEntry>>;
  using const_iterator = RibTable::const_iterator;

  void
  setFibUpdater(FibUpdater* updater);

  const_iterator
  find(const Name& prefix) const;

  Route*
  find(const Name& prefix, const Route& route) const;

  Route*
  findLongestPrefix(const Name& prefix, const Route& route) const;

  const_iterator
  begin() const
  {
    return m_rib.begin();
  }

  const_iterator
  end() const
  {
    return m_rib.end();
  }

  size_t
  size() const noexcept
  {
    return m_nItems;
  }

  [[nodiscard]] bool
  empty() const noexcept
  {
    return m_rib.empty();
  }

  shared_ptr<RibEntry>
  findParent(const Name& prefix) const;

public:
  using UpdateSuccessCallback = std::function<void()>;
  using UpdateFailureCallback = std::function<void(uint32_t code, const std::string& error)>;

  /** \brief Passes the provided RibUpdateBatch to FibUpdater to calculate and send FibUpdates.
   *
   *  If the FIB is updated successfully, onFibUpdateSuccess() will be called, and the
   *  RIB will be updated
   *
   *  If the FIB update fails, onFibUpdateFailure() will be called, and the RIB will not
   *  be updated.
   */
  void
  beginApplyUpdate(const RibUpdate& update,
                   const UpdateSuccessCallback& onSuccess,
                   const UpdateFailureCallback& onFailure);

  /** \brief Starts the FIB update process when a face has been destroyed.
   */
  void
  beginRemoveFace(uint64_t faceId);

  void
  beginRemoveFailedFaces(const std::set<uint64_t>& activeFaceIds);

  void
  onRouteExpiration(const Name& prefix, const Route& route);

  void
  insert(const Name& prefix, const Route& route);

private:
  void
  enqueueRemoveFace(const RibEntry& entry, uint64_t faceId);

  /** \brief Append the RIB update to the update queue.
   *
   *  To start updates, invoke sendBatchFromQueue() .
   */
  void
  addUpdateToQueue(const RibUpdate& update,
                   const Rib::UpdateSuccessCallback& onSuccess,
                   const Rib::UpdateFailureCallback& onFailure);

  /** \brief Send the first update batch in the queue, if no other update is in progress.
   */
  void
  sendBatchFromQueue();

  void
  onFibUpdateSuccess(const RibUpdateBatch& batch,
                     const RibUpdateList& inheritedRoutes,
                     const Rib::UpdateSuccessCallback& onSuccess);

  void
  onFibUpdateFailure(const Rib::UpdateFailureCallback& onFailure,
                     uint32_t code, const std::string& error);

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  erase(const Name& prefix, const Route& route);

private:
  using RouteComparePredicate = bool (*)(const Route&, const Route&);
  using RouteSet = std::set<Route, RouteComparePredicate>;

  /** \brief Find entries under \p prefix.
   *  \pre a RIB entry exists at \p prefix
   */
  std::list<shared_ptr<RibEntry>>
  findDescendants(const Name& prefix) const;

  /** \brief Find entries under \p prefix.
   *  \pre a RIB entry does not exist at \p prefix
   */
  std::list<shared_ptr<RibEntry>>
  findDescendantsForNonInsertedName(const Name& prefix) const;

  RibTable::iterator
  eraseEntry(RibTable::iterator it);

  void
  updateRib(const RibUpdateBatch& batch);

  /** \brief Returns routes inherited from the entry's ancestors.
   *  \return a list of inherited routes
   */
  RouteSet
  getAncestorRoutes(const RibEntry& entry) const;

  /** \brief Returns routes inherited from the parent of the name and the parent's ancestors.
   *  \note A parent is first found for the passed name before inherited routes are collected
   *  \return a list of inherited routes
   */
  RouteSet
  getAncestorRoutes(const Name& name) const;

  /** \brief Applies the passed \p inheritedRoutes and their actions to the corresponding
   *  RibEntries' inheritedRoutes lists.
   */
  void
  modifyInheritedRoutes(const RibUpdateList& inheritedRoutes);

public:
  /** \brief Signals after a RIB entry is inserted.
   *
   *  A RIB entry is inserted when the first route associated with a
   *  certain namespace is added.
   */
  signal::Signal<Rib, Name> afterInsertEntry;

  /** \brief Signals after a RIB entry is erased.
   *
   *  A RIB entry is erased when the last route associated with a
   *  certain namespace is removed.
   */
  signal::Signal<Rib, Name> afterEraseEntry;

  /** \brief Signals after a Route is added.
   */
  signal::Signal<Rib, RibRouteRef> afterAddRoute;

  /** \brief Signals before a route is removed.
   */
  signal::Signal<Rib, RibRouteRef> beforeRemoveRoute;

private:
  RibTable m_rib;
  // FaceId => Entry with Route on this face
  std::multimap<uint64_t, shared_ptr<RibEntry>> m_faceEntries;
  size_t m_nItems = 0;
  FibUpdater* m_fibUpdater = nullptr;

  struct UpdateQueueItem
  {
    RibUpdateBatch batch;
    const Rib::UpdateSuccessCallback managerSuccessCallback;
    const Rib::UpdateFailureCallback managerFailureCallback;
  };

  using UpdateQueue = std::list<UpdateQueueItem>;
  UpdateQueue m_updateBatches;
  bool m_isUpdateInProgress = false;

  friend FibUpdater;
};

std::ostream&
operator<<(std::ostream& os, const Rib& rib);

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_RIB_HPP
