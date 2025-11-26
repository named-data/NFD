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

#ifndef NFD_DAEMON_TABLE_CS_POLICY_PRIORITY_FIFO_HPP
#define NFD_DAEMON_TABLE_CS_POLICY_PRIORITY_FIFO_HPP

#include "cs-policy.hpp"

#include <ndn-cxx/util/scheduler.hpp>

#include <list>

/* 
・Content Store（CS）の置換方式のひとつ「Priority FIFO（優先度付き先入れ先出し）」を実装

【基本方針】
- 各キャッシュエントリを優先度に応じて3種類のキューに分類し FIFO で管理
- エビクト（削除）は優先度の低いキューから順に実施
- 同一エントリは常にどこか1つのキューに存在する

【使用する3つのキュー】
1. QUEUE_UNSOLICITED：要求されずに届いたデータ（最も優先度低）
2. QUEUE_STALE：期限切れ・古くなったデータ
3. QUEUE_FIFO：新鮮なデータ（ヒット率に最も寄与）

→ エビクト順序：UNSOLICITED → STALE → FIFO

【EntryInfo】
- 各エントリがどのキューに属しているかを追跡する構造体
- moveStaleEventId により、新鮮（FIFO）→古い（STALE）への自動移動をスケジューリング可能

【オーバーライド（Policy から継承）】
- doAfterInsert(): 挿入後、適切なキューに配置
- doAfterRefresh(): 更新された場合、Fresh扱いに変更
- doBeforeErase(): 削除前にキューから消す
- doBeforeUse(): キャッシュヒット時にFresh扱いへ
- evictEntries(): 上限超過時に適切なキューから削除

【内部処理メソッド】
- evictOne(): 優先度に従って1件削除
- attachQueue(): キューへ追加
- detachQueue(): キューから削除
- moveToStaleQueue(): FIFO → STALE への移動

【特徴】
- LRU と比べシンプルな管理で済む一方、
  有効期限（stale化）と unsolicited データの優先的削除により CS 有効活用を狙うポリシー
- 性能（FIFO）と健全性（stale/unsolicited優先削除）のバランスをとった設計
*/

namespace nfd::cs {
namespace priority_fifo {

using Queue = std::list<Policy::EntryRef>;

enum QueueType {
  QUEUE_UNSOLICITED,
  QUEUE_STALE,
  QUEUE_FIFO,
  QUEUE_MAX
};

struct EntryInfo
{
  QueueType queueType;
  Queue::iterator queueIt;
  ndn::scheduler::EventId moveStaleEventId;
};

/** \brief Priority First-In-First-Out (FIFO) replacement policy.
 *
 *  This policy maintains a set of cleanup queues to decide the eviction order of CS entries.
 *  The cleanup queues are three doubly linked lists that store EntryRefs.
 *  The three queues keep track of unsolicited, stale, and fresh Data packet, respectively.
 *  EntryRef is placed into, removed from, and moved between suitable queues
 *  whenever an Entry is added, removed, or has other attribute changes.
 *  Each Entry should be in exactly one queue at any moment.
 *  Within each queue, the EntryRefs are kept in first-in-first-out order.
 *  Eviction procedure exhausts the first queue before moving onto the next queue,
 *  in the order of unsolicited, stale, and fresh queue.
 */
class PriorityFifoPolicy final : public Policy
{
public:
  PriorityFifoPolicy();

  ~PriorityFifoPolicy() final;

private:
  void
  doAfterInsert(EntryRef i) final;

  void
  doAfterRefresh(EntryRef i) final;

  void
  doBeforeErase(EntryRef i) final;

  void
  doBeforeUse(EntryRef i) final;

  void
  evictEntries() final;

private:
  /** \brief Evicts one entry.
   *  \pre CS is not empty
   */
  void
  evictOne();

  /** \brief Attaches the entry to an appropriate queue.
   *  \pre the entry is not in any queue
   */
  void
  attachQueue(EntryRef i);

  /** \brief Detaches the entry from its current queue.
   *  \post the entry is not in any queue
   */
  void
  detachQueue(EntryRef i);

  /** \brief Moves an entry from FIFO queue to STALE queue.
   */
  void
  moveToStaleQueue(EntryRef i);

public:
  static constexpr std::string_view POLICY_NAME{"priority_fifo"};

private:
  Queue m_queues[QUEUE_MAX];
  std::map<EntryRef, EntryInfo*> m_entryInfoMap;
};

} // namespace priority_fifo

using priority_fifo::PriorityFifoPolicy;

} // namespace nfd::cs

#endif // NFD_DAEMON_TABLE_CS_POLICY_PRIORITY_FIFO_HPP
