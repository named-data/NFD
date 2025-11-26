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

#ifndef NFD_DAEMON_TABLE_CS_ENTRY_HPP
#define NFD_DAEMON_TABLE_CS_ENTRY_HPP

#include "core/common.hpp"

#include <set>

/*
・Content Store に格納される 1 件のデータを表すクラス（CS Entry）

【役割】
- Data パケットを保持
- Unsolicited（要求されていないデータ）かどうかを記録
- Freshness（鮮度）の管理
- Interest に対して満足可能かどうか判定

【主要関数】
- getData(): 保持している Data を返す
- getName(): Data の Name を返す（ImplicitDigest 含まない）
- getFullName(): ImplicitDigest 含む完全な Name
- isUnsolicited(): 未要求のデータか判定
- isFresh(): 現時点で FreshnessPeriod を満たすか
- canSatisfy(Interest): Interest がこの Data を満たすか判定
- updateFreshUntil(): Freshness 期限を更新
- clearUnsolicited(): Unsolicited フラグを解除

【保持データ】
- shared_ptr<const Data> m_data: 実データ
- bool m_isUnsolicited: 未要求データか否か
- time_point m_freshUntil: Freshness の期限時刻

【比較演算子】
- Entry vs Entry、Entry vs Name、Name vs Entry を比較可能
  → std::set で検索できる設計
  → Name で検索して Data エントリを高速に見つけられる

【Table 型】
- using Table = std::set<Entry, std::less<>>
  → ContentStore 内部での実際の保持コンテナ
  → 名前順に整列され、重複なし
  → iterator 同士も比較可能

【まとめ】
- Entry は CS の基本単位であり、Data の鮮度と利用可能性を管理する
- Table で ordered container として利用され、
  ポリシー（LRU、PriorityFIFO 等）がこれを参照したり並び替えたりして
  エビクション（削除）を行う
*/

namespace nfd::cs {

/** \brief A ContentStore entry.
 */
class Entry
{
public: // exposed through ContentStore enumeration
  /** \brief Return the stored Data.
   */
  const Data&
  getData() const
  {
    return *m_data;
  }

  /** \brief Return stored Data name.
   */
  const Name&
  getName() const
  {
    return m_data->getName();
  }

  /** \brief Return full name (including implicit digest) of the stored Data.
   */
  const Name&
  getFullName() const
  {
    return m_data->getFullName();
  }

  /** \brief Return whether the stored Data is unsolicited.
   */
  bool
  isUnsolicited() const
  {
    return m_isUnsolicited;
  }

  /** \brief Check if the stored Data is fresh now.
   */
  bool
  isFresh() const;

  /** \brief Determine whether Interest can be satisified by the stored Data.
   */
  bool
  canSatisfy(const Interest& interest) const;

public: // used by ContentStore implementation
  Entry(shared_ptr<const Data> data, bool isUnsolicited);

  /** \brief Recalculate when the entry would become non-fresh, relative to current time.
   */
  void
  updateFreshUntil();

  /** \brief Clear 'unsolicited' flag.
   */
  void
  clearUnsolicited()
  {
    m_isUnsolicited = false;
  }

private:
  shared_ptr<const Data> m_data;
  bool m_isUnsolicited;
  time::steady_clock::time_point m_freshUntil;
};

bool
operator<(const Entry& entry, const Name& queryName);

bool
operator<(const Name& queryName, const Entry& entry);

bool
operator<(const Entry& lhs, const Entry& rhs);

/** \brief An ordered container of ContentStore entries.
 *
 *  This container uses std::less<> comparator to enable lookup with queryName.
 */
using Table = std::set<Entry, std::less<>>;

inline bool
operator<(Table::const_iterator lhs, Table::const_iterator rhs)
{
  return *lhs < *rhs;
}

} // namespace nfd::cs

#endif // NFD_DAEMON_TABLE_CS_ENTRY_HPP
