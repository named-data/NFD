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

#ifndef NFD_DAEMON_TABLE_FIB_HPP
#define NFD_DAEMON_TABLE_FIB_HPP

#include "fib-entry.hpp"
#include "name-tree.hpp"

#include <boost/range/adaptor/transformed.hpp>

/*
 * fib.hpp 解説
 *
 * このヘッダファイルは NFD (Named Data Networking Forwarding Daemon) における
 * Forwarding Information Base (FIB) を定義しています。
 * FIB はネットワークの転送情報を保持するテーブルで、名前ベースのルーティングに
 * 使用されます。
 *
 * クラス Fib の役割:
 * - 名前に基づいてパケットを転送するための情報を管理する。
 * - 長さ優先マッチ (Longest Prefix Match, LPM) による検索を提供。
 * - FIB エントリの追加、削除、NextHop の追加や更新を行う。
 * - FIB 内のエントリの列挙（イテレーション）を可能にする。
 *
 * 主要メンバ:
 * - NameTree& m_nameTree: 名前ツリーの参照。FIB エントリを階層的に管理。
 * - size_t m_nItems: FIB に登録されているエントリ数。
 * - static const unique_ptr<Entry> s_emptyEntry: マッチしない場合に返される空エントリ。
 *
 * 主要メソッド:
 * - size(): 登録されている FIB エントリ数を返す。
 *
 * - findLongestPrefixMatch(): 指定された名前に対して最長一致の FIB エントリを返す。
 *   オーバーロードにより Name、pit::Entry、measurements::Entry に対応。
 *
 * - findExactMatch(): 名前に対して完全一致する FIB エントリを返す。
 *
 * - insert(): 指定された名前の FIB エントリを検索し、存在しなければ新規追加。
 *   戻り値はエントリポインタと新規追加フラグのペア。
 *
 * - erase(): 指定された名前またはエントリを削除。
 *
 * - addOrUpdateNextHop(): FIB エントリに NextHop を追加する。既存の場合はコストを更新。
 *
 * - removeNextHop(): 指定した Face に対応する NextHop を削除。
 *   削除結果に応じて NO_SUCH_NEXTHOP / NEXTHOP_REMOVED / FIB_ENTRY_REMOVED を返す。
 *
 * イテレーション:
 * - begin() / end() によって FIB 内のエントリを列挙可能。
 * - 列挙中に FIB を変更すると未定義動作の可能性あり。
 *
 * シグナル:
 * - afterNewNextHop: 新しい NextHop が追加された際に通知を行うためのシグナル。
 *
 * 内部処理:
 * - findLongestPrefixMatchImpl(): テンプレート実装により LPM 検索の内部処理を共通化。
 * - erase(name_tree::Entry*, bool): 内部で FIB エントリを削除する際に使用。
 * - getRange(): NameTree をラップして FIB エントリの範囲を返す。
 *
 * 注意:
 * - FIB エントリの最大深さは NameTree::getMaxDepth() で制限されている。
 * - FIB エントリは NextHop を持つことができ、各 NextHop は Face とコスト情報を保持。
 *
 * 使用例:
 *   Fib fib(nameTree);
 *   auto [entry, isNew] = fib.insert(someName);
 *   fib.addOrUpdateNextHop(*entry, face, 10);
 *   auto match = fib.findLongestPrefixMatch(pitEntry);
 */

namespace nfd {

namespace measurements {
class Entry;
} // namespace measurements

namespace pit {
class Entry;
} // namespace pit

namespace fib {

/**
 * \brief Represents the Forwarding Information Base (FIB).
 * \sa fib::Entry
 */
class Fib : noncopyable
{
public:
  explicit
  Fib(NameTree& nameTree);

  size_t
  size() const noexcept
  {
    return m_nItems;
  }

public: // lookup
  /** \brief Performs a longest prefix match.
   */
  const Entry&
  findLongestPrefixMatch(const Name& prefix) const;

  /** \brief Performs a longest prefix match.
   *
   *  This is equivalent to `findLongestPrefixMatch(pitEntry.getName())`
   */
  const Entry&
  findLongestPrefixMatch(const pit::Entry& pitEntry) const;

  /** \brief Performs a longest prefix match.
   *
   *  This is equivalent to `findLongestPrefixMatch(measurementsEntry.getName())`
   */
  const Entry&
  findLongestPrefixMatch(const measurements::Entry& measurementsEntry) const;

  /** \brief Performs an exact match lookup.
   */
  Entry*
  findExactMatch(const Name& prefix);

public: // mutation
  /** \brief Maximum number of components in a FIB entry prefix.
   */
  static constexpr size_t
  getMaxDepth()
  {
    return NameTree::getMaxDepth();
  }

  /** \brief Find or insert a FIB entry.
   *  \param prefix FIB entry name; it must not have more than \c getMaxDepth() components.
   *  \return the entry, and true for new entry or false for existing entry
   */
  std::pair<Entry*, bool>
  insert(const Name& prefix);

  void
  erase(const Name& prefix);

  void
  erase(const Entry& entry);

  /** \brief Add a NextHop record.
   *
   *  If a NextHop record for \p face already exists in \p entry, its cost is set to \p cost.
   */
  void
  addOrUpdateNextHop(Entry& entry, Face& face, uint64_t cost);

  enum class RemoveNextHopResult {
    NO_SUCH_NEXTHOP, ///< the nexthop is not found
    NEXTHOP_REMOVED, ///< the nexthop is removed and the fib entry stays
    FIB_ENTRY_REMOVED ///< the nexthop is removed and the fib entry is removed
  };

  /** \brief Remove the NextHop record for \p face from \p entry.
   */
  RemoveNextHopResult
  removeNextHop(Entry& entry, const Face& face);

public: // enumeration
  using Range = boost::transformed_range<name_tree::GetTableEntry<Entry>, const name_tree::Range>;
  using const_iterator = boost::range_iterator<Range>::type;

  /** \return an iterator to the beginning
   *  \note The iteration order is implementation-defined.
   *  \warning Undefined behavior may occur if a FIB/PIT/Measurements/StrategyChoice entry
   *           is inserted or erased during iteration.
   */
  const_iterator
  begin() const
  {
    return this->getRange().begin();
  }

  /** \return an iterator to the end
   *  \sa begin()
   */
  const_iterator
  end() const
  {
    return this->getRange().end();
  }

public: // signal
  /** \brief Signals on Fib entry nexthop creation.
   */
  signal::Signal<Fib, Name, NextHop> afterNewNextHop;

private:
  /** \tparam K a parameter acceptable to NameTree::findLongestPrefixMatch
   */
  template<typename K>
  const Entry&
  findLongestPrefixMatchImpl(const K& key) const;

  void
  erase(name_tree::Entry* nte, bool canDeleteNte = true);

  Range
  getRange() const;

private:
  NameTree& m_nameTree;
  size_t m_nItems = 0;

  /** \brief The empty FIB entry.
   *
   *  This entry has no nexthops.
   *  It is returned by findLongestPrefixMatch if nothing is matched.
   */
  static const unique_ptr<Entry> s_emptyEntry;
};

} // namespace fib

using fib::Fib;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_FIB_HPP
