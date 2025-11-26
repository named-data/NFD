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

#ifndef NFD_DAEMON_TABLE_MEASUREMENTS_ACCESSOR_HPP
#define NFD_DAEMON_TABLE_MEASUREMENTS_ACCESSOR_HPP

#include "measurements.hpp"
#include "strategy-choice.hpp"


/*
 * MeasurementsAccessor.hpp 解説
 *
 * 【概要】
 * - Forwarding Strategy（fw::Strategy）が Measurements テーブルへ
 *   安全にアクセスするためのアクセサクラス。
 * - StrategyChoiceが定めた戦略の「権限範囲内の名前空間」だけを参照できる。
 *
 * 【役割】
 * - Measurementsテーブルへの操作をラップし、
 *   Strategyがアクセスしてよいエントリだけ返すフィルタ機能を提供
 * - 戦略ごとのNamespace（プレフィックス領域）分離を実現
 *
 * 【提供する操作】
 * 1. get(name/fibEntry/pitEntry)
 *    - Measurementsエントリを取得、なければ作成
 *
 * 2. getParent(childEntry)
 *    - 子のプレフィックスの親におけるMeasurementsエントリ取得
 *
 * 3. findLongestPrefixMatch(...)
 *    - 最長一致検索（pred条件でフィルタ可能）
 *
 * 4. findExactMatch(name)
 *    - 完全一致検索
 *
 * 5. extendLifetime(entry, lifetime)
 *    - エントリの有効期限延長（アクセス更新）
 *
 * ※ 上記すべて「戦略がアクセス権を持つ」場合にのみ有効
 *   →権限外は filter() によって nullptr を返す
 *
 * 【主要メンバ】
 * - Measurements& m_measurements
 *     操作対象の Measurements テーブル本体
 *
 * - const StrategyChoice& m_strategyChoice
 *     どの名前空間をどの戦略が担当するかの管理
 *
 * - const fw::Strategy* m_strategy
 *     アクセス元戦略（権限チェックに使用）
 *
 * 【ポイント】
 * - noncopyable：コピー禁止、参照運用が前提
 * - StrategyChoice による Namespace 分割を厳密に遵守
 * - “テーブル構造” を知らずに Strategy が利用できる抽象化インタフェース
 *
 * 【設計意図】
 * - Strategy が Measurements テーブルへ直接触るのを防ぎ、
 *   誤ったプレフィックスへのアクセスを制限
 * - 複数戦略が共存する環境で名前空間を安全に隔離
 *
 * 【利用例のイメージ】
 * - Interest処理で、Strategyが利用頻度統計を記録
 * - 成功/失敗情報を測定 → 名前空間ごとの最適な転送判断に活用
 */

namespace nfd {

namespace fw {
class Strategy;
} // namespace fw

namespace measurements {

/**
 * \brief Allows fw::Strategy to access the portion of Measurements table under its namespace.
 *
 * All public methods have the same semantics as the corresponding methods on Measurements,
 * but will return nullptr if the entry falls out of the strategy's authority.
 */
class MeasurementsAccessor : noncopyable
{
public:
  MeasurementsAccessor(Measurements& measurements, const StrategyChoice& strategyChoice,
                       const fw::Strategy& strategy);

  ~MeasurementsAccessor();

  /** \brief Find or insert a Measurements entry for \p name.
   */
  Entry*
  get(const Name& name);

  /** \brief Find or insert a Measurements entry for \p fibEntry->getPrefix().
   */
  Entry*
  get(const fib::Entry& fibEntry);

  /** \brief Find or insert a Measurements entry for \p pitEntry->getName().
   */
  Entry*
  get(const pit::Entry& pitEntry);

  /** \brief Find or insert a Measurements entry for child's parent.
   */
  Entry*
  getParent(const Entry& child);

  /** \brief Perform a longest prefix match for \p name.
   */
  Entry*
  findLongestPrefixMatch(const Name& name,
                         const EntryPredicate& pred = AnyEntry()) const;

  /** \brief Perform a longest prefix match for \p pitEntry.getName().
   */
  Entry*
  findLongestPrefixMatch(const pit::Entry& pitEntry,
                         const EntryPredicate& pred = AnyEntry()) const;

  /** \brief Perform an exact match.
   */
  Entry*
  findExactMatch(const Name& name) const;

  /** \brief Extend lifetime of an entry.
   *
   *  The entry will be kept until at least now()+lifetime.
   */
  void
  extendLifetime(Entry& entry, const time::nanoseconds& lifetime);

private:
  /** \brief Perform access control to Measurements entry.
   *  \return entry if strategy has access to namespace, otherwise nullptr
   */
  Entry*
  filter(Entry* entry) const;

  Entry*
  filter(Entry& entry) const;

private:
  Measurements& m_measurements;
  const StrategyChoice& m_strategyChoice;
  const fw::Strategy* m_strategy;
};

inline Entry*
MeasurementsAccessor::filter(Entry& entry) const
{
  return this->filter(&entry);
}

inline Entry*
MeasurementsAccessor::get(const Name& name)
{
  return this->filter(m_measurements.get(name));
}

inline Entry*
MeasurementsAccessor::get(const fib::Entry& fibEntry)
{
  return this->filter(m_measurements.get(fibEntry));
}

inline Entry*
MeasurementsAccessor::get(const pit::Entry& pitEntry)
{
  return this->filter(m_measurements.get(pitEntry));
}

inline Entry*
MeasurementsAccessor::getParent(const Entry& child)
{
  return this->filter(m_measurements.getParent(child));
}

inline Entry*
MeasurementsAccessor::findLongestPrefixMatch(const Name& name,
                                             const EntryPredicate& pred) const
{
  return this->filter(m_measurements.findLongestPrefixMatch(name, pred));
}

inline Entry*
MeasurementsAccessor::findLongestPrefixMatch(const pit::Entry& pitEntry,
                                             const EntryPredicate& pred) const
{
  return this->filter(m_measurements.findLongestPrefixMatch(pitEntry, pred));
}

inline Entry*
MeasurementsAccessor::findExactMatch(const Name& name) const
{
  return this->filter(m_measurements.findExactMatch(name));
}

inline void
MeasurementsAccessor::extendLifetime(Entry& entry, const time::nanoseconds& lifetime)
{
  m_measurements.extendLifetime(entry, lifetime);
}

} // namespace measurements

using measurements::MeasurementsAccessor;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_MEASUREMENTS_ACCESSOR_HPP
