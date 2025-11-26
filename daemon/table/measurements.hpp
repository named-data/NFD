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

#ifndef NFD_DAEMON_TABLE_MEASUREMENTS_HPP
#define NFD_DAEMON_TABLE_MEASUREMENTS_HPP

#include "measurements-entry.hpp"
#include "name-tree.hpp"

#include <functional>

/*
 * measurements.hpp 解説
 *
 * このファイルは NFD (Named Data Networking Forwarding Daemon) の
 * Measurements Table を定義している。
 *
 * Measurements Table の役割:
 * - Forwarding Strategy が名前プレフィックスごとに計測・学習した情報
 *   （遅延、成功率、Face の優先度など）を保存するデータベース。
 * - 情報は StrategyInfo を継承するクラスとして格納される。
 * - 名前階層ごとに探索でき、最長一致探索が可能（Longest Prefix Match）。
 *
 * 関連概念:
 * - FIB と類似した階層構造だが、「制御情報や学習情報」を保持する点が異なる。
 * - PIT/FIB と連携し、より適切な転送戦略を実現するための補助機能。
 *
 * ----------------------------------------------
 * 主なメンバ変数
 * ----------------------------------------------
 * - NameTree& m_nameTree:
 *     名前階層を表す NameTree を参照し、Measurements エントリを管理。
 *
 * - size_t m_nItems:
 *     エントリ数。メモリ管理などに利用。
 *
 * ----------------------------------------------
 * EntryPredicate:
 * - Entry を条件付きで受け入れるフィルタ関数型。
 * - Strategy 情報の有無などで探索対象を絞る場合に利用。
 *
 * Examples:
 *   AnyEntry: すべての Entry を有効
 *   EntryWithStrategyInfo<T>: 特定の StrategyInfo<T> が存在する Entry のみ一致
 *
 * ----------------------------------------------
 * 主なメソッド
 * ----------------------------------------------
 * - get(const Name&):
 *     指定した名前のエントリを取得、存在しなければ追加。
 *     深さ制限 (getMaxDepth) を超える場合は切り詰める。
 *
 * - get(const fib::Entry&) / get(const pit::Entry&):
 *     FIB / PIT の情報から Measurements エントリを取得。
 *
 * - getParent(const Entry&):
 *     親階層の Measurements エントリを参照。
 *
 * - findLongestPrefixMatch(...):
 *     名前の最長一致検索。
 *     EntryPredicate を指定することで条件検索が可能。
 *
 * - findExactMatch(const Name&):
 *     完全一致するエントリを返す。なければ nullptr。
 *
 * - extendLifetime(Entry&, lifetime):
 *     エントリの有効期限を延長。自動クリーンアップを調整。
 *
 * - size():
 *     現在のエントリ数を返す。
 *
 * ----------------------------------------------
 * 内部処理
 * ----------------------------------------------
 * - cleanup(): 期限切れのエントリを削除。
 * - findLongestPrefixMatchImpl(): LPM 探索内部処理。
 * - get(name_tree::Entry&): NameTree のエントリを Measurements Entry に変換。
 *
 * ----------------------------------------------
 * 使用例
 * ----------------------------------------------
 *   Measurements ms(nameTree);
 *   auto& e = ms.get(Name("/example/test"));
 *   ms.extendLifetime(e, 1_s);
 *   auto* match = ms.findLongestPrefixMatch(Name("/example"));
 */

namespace nfd {

namespace fib {
class Entry;
} // namespace fib

namespace pit {
class Entry;
} // namespace pit

namespace measurements {

/**
 * \brief A predicate that accepts or rejects an entry.
 */
using EntryPredicate = std::function<bool(const Entry&)>;

/**
 * \brief An #EntryPredicate that accepts any entry.
 */
class AnyEntry
{
public:
  constexpr bool
  operator()(const Entry&) const noexcept
  {
    return true;
  }
};

/**
 * \brief An #EntryPredicate that accepts an entry if it has StrategyInfo of type T.
 */
template<typename T>
class EntryWithStrategyInfo
{
public:
  bool
  operator()(const Entry& entry) const
  {
    return entry.getStrategyInfo<T>() != nullptr;
  }
};

/**
 * \brief The %Measurements table.
 *
 * The %Measurements table is a data structure for forwarding strategies to store per name prefix
 * measurements. A strategy can access this table via fw::Strategy::getMeasurements(), and then
 * place any object that derive from StrategyInfo type onto %Measurements entries.
 */
class Measurements : noncopyable
{
public:
  explicit
  Measurements(NameTree& nameTree);

  /** \brief Maximum depth of a %Measurements entry.
   */
  static constexpr size_t
  getMaxDepth()
  {
    return NameTree::getMaxDepth();
  }

  /** \brief Find or insert an entry by name.
   *
   *  An entry name can have at most getMaxDepth() components. If \p name exceeds this limit,
   *  it is truncated to the first getMaxDepth() components.
   */
  Entry&
  get(const Name& name);

  /** \brief Equivalent to `get(fibEntry.getPrefix())`.
   */
  Entry&
  get(const fib::Entry& fibEntry);

  /** \brief Equivalent to `get(pitEntry.getName(), std::min(pitEntry.getName().size(), getMaxDepth()))`.
   */
  Entry&
  get(const pit::Entry& pitEntry);

  /** \brief Find or insert a parent entry.
   *  \retval nullptr child is the root entry
   *  \return get(child.getName().getPrefix(-1))
   */
  Entry*
  getParent(const Entry& child);

  /** \brief Perform a longest prefix match for \p name.
   */
  Entry*
  findLongestPrefixMatch(const Name& name,
                         const EntryPredicate& pred = AnyEntry()) const;

  /** \brief Perform a longest prefix match for `pitEntry.getName()`.
   */
  Entry*
  findLongestPrefixMatch(const pit::Entry& pitEntry,
                         const EntryPredicate& pred = AnyEntry()) const;

  /** \brief Perform an exact match.
   */
  Entry*
  findExactMatch(const Name& name) const;

  static time::nanoseconds
  getInitialLifetime()
  {
    return 4_s;
  }

  /** \brief Extend lifetime of an entry.
   *
   *  The entry will be kept until at least now()+lifetime.
   */
  void
  extendLifetime(Entry& entry, const time::nanoseconds& lifetime);

  size_t
  size() const
  {
    return m_nItems;
  }

private:
  void
  cleanup(Entry& entry);

  Entry&
  get(name_tree::Entry& nte);

  /** \tparam K a parameter acceptable to NameTree::findLongestPrefixMatch()
   */
  template<typename K>
  Entry*
  findLongestPrefixMatchImpl(const K& key, const EntryPredicate& pred) const;

private:
  NameTree& m_nameTree;
  size_t m_nItems = 0;
};

} // namespace measurements

using measurements::Measurements;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_MEASUREMENTS_HPP
